// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! A networking stack.

// In case we roll the toolchain and something we're using as a feature has been
// stabilized.
#![allow(stable_features)]
#![feature(specialization)]
#![deny(missing_docs)]
#![deny(unreachable_patterns)]
// TODO(joshlf): Remove this once all of the elements in the crate are actually
// used.
#![allow(unused)]
#![deny(unused_imports)]
// This is a hack until we migrate to a different benchmarking framework. To run
// benchmarks, edit your Cargo.toml file to add a "benchmark" feature, and then
// run with that feature enabled.
#![cfg_attr(feature = "benchmark", feature(test))]

#[cfg(all(test, feature = "benchmark"))]
extern crate test;

// TODO(joshlf): Remove this once the old packet crate has been deleted and the
// new one's name has been changed back to `packet`.
extern crate packet_new as packet;

#[macro_use]
mod macros;

#[cfg(test)]
mod benchmarks;
mod context;
mod data_structures;
mod device;
mod error;
mod ip;
#[cfg(test)]
mod testutil;
mod transport;
mod wire;

use log::trace;

pub use crate::data_structures::{IdMapCollection, IdMapCollectionKey};
pub use crate::device::{
    get_ip_addr_subnet, initialize_device, receive_frame, DeviceId, DeviceLayerEventDispatcher,
};
pub use crate::error::NetstackError;
pub use crate::ip::{
    icmp, EntryDest, EntryDestEither, EntryEither, IpLayerEventDispatcher, IpStateBuilder,
};
pub use crate::transport::udp::UdpEventDispatcher;
pub use crate::transport::TransportLayerEventDispatcher;

use net_types::ethernet::Mac;
use net_types::ip::{AddrSubnetEither, Ipv4Addr, Ipv6Addr, SubnetEither};
use packet::{Buf, BufferMut, EmptyBuf};
use rand::{CryptoRng, RngCore};
use std::time;

use crate::device::{DeviceLayerState, DeviceLayerTimerId, DeviceStateBuilder};
use crate::ip::{IpLayerState, IpLayerTimerId};
use crate::transport::{TransportLayerState, TransportLayerTimerId};

/// Map an expression over either version of an address.
///
/// `map_addr_version!` takes a type which is an enum with two variants - `V4`
/// and `V6` - and a value of that type. It matches on the variants, and for
/// both variants, invokes an expression on the inner contents. `$addr` is both
/// the name of the variable to match on, and the name that the address will be
/// bound to for the scope of the expression.
///
/// To make it concrete, the expression `map_addr_version!(Foo, bar, blah(bar))`
/// desugars to:
///
/// ```rust,ignore
/// match bar {
///     Foo::V4(bar) => blah(bar),
///     Foo::V6(bar) => blah(bar),
/// }
/// ```
#[macro_export]
macro_rules! map_addr_version {
    ($ty:tt, $addr:ident, $expr:expr) => {
        match $addr {
            $ty::V4($addr) => $expr,
            $ty::V6($addr) => $expr,
        }
    };
    ($ty:tt, $addr:ident, $expr:expr,) => {
        map_addr_version!($addr, $expr)
    };
}

/// A builder for [`StackState`].
#[derive(Default, Clone)]
pub struct StackStateBuilder {
    ip: IpStateBuilder,
    device: DeviceStateBuilder,
}

impl StackStateBuilder {
    /// Get the builder for the IP state.
    pub fn ip_builder(&mut self) -> &mut IpStateBuilder {
        &mut self.ip
    }

    /// Get the builder for the device state.
    pub fn device_builder(&mut self) -> &mut DeviceStateBuilder {
        &mut self.device
    }

    /// Consume this builder and produce a `StackState`.
    pub fn build<D: EventDispatcher>(self) -> StackState<D> {
        StackState {
            transport: TransportLayerState::default(),
            ip: self.ip.build(),
            device: self.device.build(),
            #[cfg(test)]
            test_counters: testutil::TestCounters::default(),
        }
    }
}

/// The state associated with the network stack.
pub struct StackState<D: EventDispatcher> {
    transport: TransportLayerState,
    ip: IpLayerState<D>,
    device: DeviceLayerState,
    #[cfg(test)]
    test_counters: testutil::TestCounters,
}

impl<D: EventDispatcher> StackState<D> {
    /// Add a new ethernet device to the device layer.
    ///
    /// `add_ethernet_device` only makes the netstack aware of the device. The device still needs to
    /// be initialized. A device MUST NOT be used until it has been initialized. The netstack
    /// promises not to generate any outbound traffic on the device until [`initialize_device`] has
    /// been called.
    ///
    /// See [`initialize_device`] for more information.
    ///
    /// [`initialize_device`]: crate::device::initialize_device
    pub fn add_ethernet_device(&mut self, mac: Mac, mtu: u32) -> DeviceId {
        self.device.add_ethernet_device(mac, mtu)
    }
}

impl<D: EventDispatcher> Default for StackState<D> {
    fn default() -> StackState<D> {
        StackStateBuilder::default().build()
    }
}

/// Context available during the execution of the netstack.
///
/// `Context` provides access to the state of the netstack and to an event
/// dispatcher which can be used to emit events and schedule timeouts. A mutable
/// reference to a `Context` is passed to every function in the netstack.
#[derive(Default)]
pub struct Context<D: EventDispatcher> {
    state: StackState<D>,
    dispatcher: D,
}

impl<D: EventDispatcher> Context<D> {
    /// Construct a new `Context`.
    pub fn new(state: StackState<D>, dispatcher: D) -> Context<D> {
        Context { state, dispatcher }
    }

    /// Construct a new `Context` using the default `StackState`.
    pub fn with_default_state(dispatcher: D) -> Context<D> {
        Context { state: StackState::default(), dispatcher }
    }

    /// Get the stack state immutably.
    pub fn state(&self) -> &StackState<D> {
        &self.state
    }

    /// Get the stack state mutably.
    pub fn state_mut(&mut self) -> &mut StackState<D> {
        &mut self.state
    }

    /// Get the dispatcher immutably.
    pub fn dispatcher(&self) -> &D {
        &self.dispatcher
    }

    /// Get the dispatcher mutably.
    pub fn dispatcher_mut(&mut self) -> &mut D {
        &mut self.dispatcher
    }

    /// Get the stack state and the dispatcher.
    ///
    /// This is useful when a mutable reference to both are required at the same
    /// time, which isn't possible when using the `state` or `dispatcher`
    /// methods.
    pub fn state_and_dispatcher(&mut self) -> (&mut StackState<D>, &mut D) {
        (&mut self.state, &mut self.dispatcher)
    }
}

impl<D: EventDispatcher + Default> Context<D> {
    /// Construct a new `Context` using the default dispatcher.
    pub fn with_default_dispatcher(state: StackState<D>) -> Context<D> {
        Context { state, dispatcher: D::default() }
    }
}

/// The identifier for any timer event.
#[derive(Copy, Clone, Eq, PartialEq, Debug, Hash)]
pub struct TimerId(TimerIdInner);

#[derive(Copy, Clone, Eq, PartialEq, Debug, Hash)]
enum TimerIdInner {
    /// A timer event in the device layer.
    DeviceLayer(DeviceLayerTimerId),
    /// A timer event in the transport layer.
    TransportLayer(TransportLayerTimerId),
    /// A timer event in the IP layer.
    IpLayer(IpLayerTimerId),
    /// A no-op timer event (used for tests)
    #[cfg(test)]
    Nop(usize),
}

impl From<DeviceLayerTimerId> for TimerId {
    fn from(id: DeviceLayerTimerId) -> TimerId {
        TimerId(TimerIdInner::DeviceLayer(id))
    }
}

/// Handle a generic timer event.
pub fn handle_timeout<D: EventDispatcher>(ctx: &mut Context<D>, id: TimerId) {
    trace!("handle_timeout: dispatching timerid: {:?}", id);

    match id {
        TimerId(TimerIdInner::DeviceLayer(x)) => {
            device::handle_timeout(ctx, x);
        }
        TimerId(TimerIdInner::TransportLayer(x)) => {
            transport::handle_timeout(ctx, x);
        }
        TimerId(TimerIdInner::IpLayer(x)) => {
            ip::handle_timeout(ctx, x);
        }
        #[cfg(test)]
        TimerId(TimerIdInner::Nop(_)) => {
            increment_counter!(ctx, "timer::nop");
        }
    }
}

/// A type representing an instant in time.
///
/// `Instant` can be implemented by any type which represents an instant in
/// time. This can include any sort of real-world clock time (e.g.,
/// [`std::time::Instant`]) or fake time such as in testing.
pub trait Instant: Sized + Ord + Copy + Clone {
    /// Returns the amount of time elapsed from another instant to this one.
    ///
    /// # Panics
    ///
    /// This function will panic if `earlier` is later than `self`.
    fn duration_since(&self, earlier: Self) -> time::Duration;

    /// Returns `Some(t)` where `t` is the time `self + duration` if `t` can be
    /// represented as `Instant` (which means it's inside the bounds of the
    /// underlying data structure), `None` otherwise.
    fn checked_add(&self, duration: time::Duration) -> Option<Self>;

    /// Returns `Some(t)` where `t` is the time `self - duration` if `t` can be
    /// represented as `Instant` (which means it's inside the bounds of the
    /// underlying data structure), `None` otherwise.
    fn checked_sub(&self, duration: time::Duration) -> Option<Self>;
}

impl Instant for time::Instant {
    fn duration_since(&self, earlier: time::Instant) -> time::Duration {
        time::Instant::duration_since(self, earlier)
    }

    fn checked_add(&self, duration: time::Duration) -> Option<Self> {
        time::Instant::checked_add(self, duration)
    }

    fn checked_sub(&self, duration: time::Duration) -> Option<Self> {
        time::Instant::checked_sub(self, duration)
    }
}

/// An `EventDispatcher` which supports sending buffers of a given type.
///
/// `D: BufferDispatcher<B>` is shorthand for `D: EventDispatcher +
/// DeviceLayerEventDispatcher<B>`.
pub trait BufferDispatcher<B: BufferMut>: EventDispatcher + DeviceLayerEventDispatcher<B> {}
impl<B: BufferMut, D: EventDispatcher + DeviceLayerEventDispatcher<B>> BufferDispatcher<B> for D {}

// TODO(joshlf): Should we add a `for<'a> DeviceLayerEventDispatcher<&'a mut
// [u8]>` bound? Would anything get more efficient if we were able to stack
// allocate internally-generated buffers?

/// An object which can dispatch events to a real system.
///
/// An `EventDispatcher` provides access to a real system. It provides the
/// ability to emit events and schedule timeouts. Each layer of the stack
/// provides its own event dispatcher trait which specifies the types of actions
/// that must be supported in order to support that layer of the stack. The
/// `EventDispatcher` trait is a sub-trait of all of these traits.
pub trait EventDispatcher:
    DeviceLayerEventDispatcher<Buf<Vec<u8>>>
    + DeviceLayerEventDispatcher<EmptyBuf>
    + IpLayerEventDispatcher
    + TransportLayerEventDispatcher
{
    /// The type of an instant in time.
    ///
    /// All time is measured using `Instant`s, including scheduling timeouts.
    /// This type may represent some sort of real-world time (e.g.,
    /// [`std::time::Instant`]), or may be mocked in testing using a fake clock.
    type Instant: Instant;

    /// Returns the current instant.
    ///
    /// `now` guarantees that two subsequent calls to `now` will return monotonically
    /// non-decreasing values.
    fn now(&self) -> Self::Instant;

    /// Schedule a callback to be invoked after a timeout.
    ///
    /// `schedule_timeout` schedules `f` to be invoked after `duration` has
    /// elapsed, overwriting any previous timeout with the same ID.
    ///
    /// If there was previously a timer with that ID, return the time at which
    /// is was scheduled to fire.
    ///
    /// # Panics
    ///
    /// `schedule_timeout` may panic if `duration` is large enough that
    /// `self.now() + duration` overflows.
    fn schedule_timeout(&mut self, duration: time::Duration, id: TimerId) -> Option<Self::Instant> {
        self.schedule_timeout_instant(self.now().checked_add(duration).unwrap(), id)
    }

    /// Schedule a callback to be invoked at a specific time.
    ///
    /// `schedule_timeout_instant` schedules `f` to be invoked at `time`,
    /// overwriting any previous timeout with the same ID.
    ///
    /// If there was previously a timer with that ID, return the time at which
    /// is was scheduled to fire.
    fn schedule_timeout_instant(
        &mut self,
        time: Self::Instant,
        id: TimerId,
    ) -> Option<Self::Instant>;

    /// Cancel a timeout.
    ///
    /// Returns true if the timeout was cancelled, false if there was no timeout
    /// for the given ID.
    fn cancel_timeout(&mut self, id: TimerId) -> Option<Self::Instant>;

    /// Cancel all timeouts which satisfy a predicate.
    ///
    /// `cancel_timeouts_with` calls `f` on each scheduled timer, and cancels
    /// any timeout for which `f` returns true.
    fn cancel_timeouts_with<F: FnMut(&TimerId) -> bool>(&mut self, f: F);

    // TODO(joshlf): If the CSPRNG requirement becomes a performance problem,
    // introduce a second, non-cryptographically secure, RNG.

    /// The random number generator (RNG) provided by this `EventDispatcher`.
    ///
    /// Code in the core is required to only obtain random values through this
    /// RNG. This allows a deterministic RNG to be provided when useful (for
    /// example, in tests).
    ///
    /// The provided RNG must be cryptographically secure in order to ensure
    /// that random values produced within the network stack are not predictable
    /// by outside observers. This helps to prevent certain kinds of
    /// fingerprinting and denial of service attacks.
    type Rng: RngCore + CryptoRng;

    /// Get the random number generator (RNG).
    ///
    /// Code in the core is required to only obtain random values through this
    /// RNG. This allows a deterministic RNG to be provided when useful (for
    /// example, in tests).
    fn rng(&mut self) -> &mut Self::Rng;
}

/// Set the IP address and subnet for a device.
pub fn set_ip_addr_subnet<D: EventDispatcher>(
    ctx: &mut Context<D>,
    device: DeviceId,
    addr_sub: AddrSubnetEither,
) {
    map_addr_version!(
        AddrSubnetEither,
        addr_sub,
        crate::device::set_ip_addr_subnet(ctx, device, addr_sub)
    );
}

/// Adds a route to the forwarding table.
pub fn add_route<D: EventDispatcher>(
    ctx: &mut Context<D>,
    entry: EntryEither,
) -> Result<(), error::NetstackError> {
    let (subnet, dest) = entry.into_subnet_dest();
    match dest {
        EntryDest::Local { device } => map_addr_version!(
            SubnetEither,
            subnet,
            crate::ip::add_device_route(ctx, subnet, device)
        )
        .map_err(From::from),
        _ => unimplemented!("Non-local route destinations not supported yet."),
    }
}

/// Delete a route from the forwarding table, returning `Err` if no
/// route was found to be deleted.
pub fn del_device_route<D: EventDispatcher>(
    ctx: &mut Context<D>,
    subnet: SubnetEither,
) -> Result<(), error::NetstackError> {
    map_addr_version!(SubnetEither, subnet, crate::ip::del_device_route(ctx, subnet))
        .map_err(From::from)
}

/// Get all the routes.
pub fn get_all_routes<'a, D: EventDispatcher>(
    ctx: &'a Context<D>,
) -> impl 'a + Iterator<Item = EntryEither> {
    let v4_routes = ip::iter_routes::<_, Ipv4Addr>(ctx);
    let v6_routes = ip::iter_routes::<_, Ipv6Addr>(ctx);
    v4_routes.cloned().map(From::from).chain(v6_routes.cloned().map(From::from))
}
