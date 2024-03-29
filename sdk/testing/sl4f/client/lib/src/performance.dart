// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:convert';
import 'dart:io' show File, Platform, Process, ProcessResult;

import 'package:logging/logging.dart';
import 'package:meta/meta.dart';

import 'dump.dart';
import 'sl4f_client.dart';

String _traceExtension({bool binary, bool compress}) {
  String extension = 'json';
  if (binary) {
    extension = 'fxt';
  }
  if (compress) {
    extension += '.gz';
  }
  return extension;
}

String _traceNameToTargetPath(String traceName, String extension) {
  return '/tmp/$traceName-trace.$extension';
}

final _log = Logger('Performance');

class Performance {
  // Environment variable names used by the catapult converter to tag the test results.
  static const String _builderNameVarName = 'BUILDER_NAME';
  static const String _buildBucketIdVarName = 'BUILDBUCKET_ID';
  static const String _buildCreateTimeVarName = 'BUILD_CREATE_TIME';
  static const String _inputCommitHostVarName = 'INPUT_COMMIT_HOST';
  static const String _inputCommitProjectVarName = 'INPUT_COMMIT_PROJECT';
  static const String _inputCommitRefVarName = 'INPUT_COMMIT_REF';

  final Sl4f _sl4f;
  final Dump _dump;

  /// Constructs a [Performance] object.
  Performance(this._sl4f, [Dump dump]) : _dump = dump ?? Dump();

  /// Closes the underlying HTTP client.
  ///
  /// This need not be called if the Sl4f client is closed instead.
  void close() {
    _sl4f.close();
  }

  /// Starts tracing for the given [duration].
  ///
  /// If [binary] is true, then the trace will be captured in Fuchsia Trace
  /// Format (by default, it is in Chrome JSON Format). If [compress] is true,
  /// the trace will be gzip-compressed. The trace output will be saved to a
  /// path implied by [traceName], [binary], and [compress], and can be
  /// retrieved later via [downloadTraceFile].
  Future<bool> trace(
      {@required Duration duration,
      @required String traceName,
      String categories,
      int bufferSize,
      bool binary = false,
      bool compress = false}) async {
    // Invoke `/bin/trace record --duration=$duration --categories=$categories
    // --output-file=$outputFile --buffer-size=$bufferSize` on the target
    // device via ssh.
    final durationSeconds = duration.inSeconds;
    String command = 'trace record --duration=$durationSeconds';
    if (categories != null) {
      command += ' --categories=$categories';
    }
    if (bufferSize != null) {
      command += ' --buffer-size=$bufferSize';
    }
    if (binary) {
      command += ' --binary';
    }
    if (compress) {
      command += ' --compress';
    }
    final String extension =
        _traceExtension(binary: binary, compress: compress);
    final outputFile = _traceNameToTargetPath(traceName, extension);
    if (outputFile != null) {
      command += ' --output-file=$outputFile';
    }
    final result = await _sl4f.ssh.run(command);
    return result.exitCode == 0;
  }

  /// Copies the trace file specified by [traceName] off of the target device,
  /// and then saves it to the dump directory.
  ///
  /// A [trace] call with the same [traceName], [binary], and [compress] must
  /// have successfully completed before calling [downloadTraceFile].
  ///
  /// Returns the download trace [File].
  Future<File> downloadTraceFile(String traceName,
      {bool binary = false, bool compress = false}) async {
    _log.info('Performance: Downloading trace $traceName');
    final String extension =
        _traceExtension(binary: binary, compress: compress);
    final tracePath = _traceNameToTargetPath(traceName, extension);
    final String response = await _sl4f
        .request('traceutil_facade.GetTraceFile', {'path': tracePath});
    return _dump.writeAsBytes(
        '$traceName-trace', extension, base64.decode(response));
  }

  /// A helper function that runs a process with the given args.
  /// Required by the test to capture the parameters passed to [Process.run].
  ///
  /// Returns [true] if the process ran successufly, [false] otherwise.
  Future<bool> runProcess(String executablePath, List<String> args) async {
    final ProcessResult results = await Process.run(executablePath, args);
    _log..info(results.stdout)..info(results.stderr);
    return results.exitCode == 0;
  }

  /// Runs the provided processor ([processorPath]) on the given [trace].
  /// It sets the ouptut file location to be the same as the source.
  /// It will also run the catapult converter if the [converterPath] was provided.
  ///
  /// The [converterPath] must be relative to the script path.
  ///
  /// TODO(PT-179): Use a processor registry instead of passing the [processorPath].
  /// TODO(PT-216): Avoid explicitly passing the [converterPath].
  ///
  /// Returns the benchmark result [File] generated by the processor.
  Future<File> processTrace(String processorPath, File trace, String testName,
      {String appName, String converterPath}) async {
    _log.info('Processing trace: ${trace.path} using $processorPath.');
    final outputFileName =
        '${trace.parent.absolute.path}/$testName-benchmark.fuchsiaperf.json';
    final List<String> args = [
      '-test_suite_name=$testName',
      if (appName != null) '-flutter_app_name=$appName',
      '-benchmarks_out_filename=$outputFileName',
      trace.absolute.path
    ];
    final processor = Platform.script.resolve(processorPath).toFilePath();
    if (!await runProcess(processor, args)) {
      _log.warning('Running the trace processor failed.');
      return null;
    }
    File processedResultFile = File(outputFileName);
    _log.info('Processing trace completed.');
    if (converterPath != null)
      await convertResults(
          converterPath, processedResultFile, Platform.environment);
    return Future.value(processedResultFile);
  }

  /// A helper function that converts the results to the catapult format.
  ///
  /// Returns the converted benchmark result [File].
  Future<File> convertResults(String converterPath, File result,
      Map<String, String> environment) async {
    _log.info('Converting the results into the catapult format');

    var bot = '', logurl = '', master = '', timestamp = 0;
    if (!environment.containsKey(_buildBucketIdVarName)) {
      _log.info(
          'convertResults: No $_buildBucketIdVarName, treating as a local run.');
      bot = 'local-bot';
      master = 'local-master';
      logurl = 'http://ci.example.com/build/300';
      timestamp = new DateTime.now().millisecondsSinceEpoch;
    } else {
      // Verify that all required environment variables are available.
      final builderName = environment[_builderNameVarName];
      final buildbucketId = environment[_buildBucketIdVarName];
      final buildCreateTime = environment[_buildCreateTimeVarName];
      final inputCommitRef = environment[_inputCommitRefVarName];
      final inputCommitHost = environment[_inputCommitHostVarName];
      final inputCommitProject = environment[_inputCommitProjectVarName];
      if (buildbucketId == null ||
          builderName == null ||
          buildCreateTime == null ||
          inputCommitRef == null ||
          inputCommitHost == null ||
          inputCommitProject == null) {
        _log.warning('Some required environment variables are not available. '
            'Current available variables are: ${environment.keys}');
        return null;
      }

      logurl = 'https://ci.chromium.org/b/$buildbucketId';
      bot = builderName;
      timestamp = int.parse(buildCreateTime);
      master =
          '${inputCommitHost.replaceFirst('.googlesource.com', '')}.$inputCommitProject';

      const releasesRefPrefix = 'refs/heads/releases/';
      if (inputCommitRef.startsWith(releasesRefPrefix)) {
        master += '.${inputCommitRef.substring(releasesRefPrefix.length)}';
      } else {
        assert(inputCommitRef == 'refs/heads/master');
      }
    }

    final resultsPath = result.absolute.path;
    assert(resultsPath.endsWith('.fuchsiaperf.json'));
    final outputFileName = resultsPath.replaceFirst(
        RegExp(r'\.fuchsiaperf\.json$'), '.catapult_json');

    final List<String> args = [
      '--input',
      result.absolute.path,
      '--output',
      outputFileName,
      '--execution-timestamp-ms',
      timestamp.toString(),
      '--masters',
      master,
      '--log-url',
      logurl,
      '--bots',
      bot
    ];

    final converter = Platform.script.resolve(converterPath).toFilePath();

    if (!await runProcess(converter, args)) {
      _log.warning('Running the results converter failed.');
      return null;
    }
    _log.info('Conversion to catapult results format completed.');
    return Future.value(File(outputFileName));
  }
}
