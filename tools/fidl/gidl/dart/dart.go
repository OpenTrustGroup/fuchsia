// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package dart

import (
	"fmt"
	"io"
	"strconv"
	"strings"
	"text/template"

	fidlir "fidl/compiler/backend/types"
	gidlir "gidl/ir"
	gidlmixer "gidl/mixer"
)

var tmpl = template.Must(template.New("tmpls").Parse(`
import 'dart:typed_data';

import 'package:test/test.dart';
import 'package:fidl/fidl.dart' as fidl;

import 'conformance_test_types.dart';
import 'gidl.dart';

void main() {
	group('conformance', () {
		group('success cases', () {
	{{ range .SuccessCases }}
	SuccessCase.run(
		{{.Name}},
		{{.Value}},
		{{.ValueType}},
		{{.Bytes}});
	{{ end }}
		});

		group('encode failure cases', () {
	{{ range .EncodeFailureCases }}
	EncodeFailureCase.run(
		{{.Name}},
		{{.Value}},
		{{.ValueType}},
		{{.ErrorCode}});
	{{ end }}
		});

		group('decode failure cases', () {
	{{ range .DecodeFailureCases }}
	DecodeFailureCase.run(
		{{.Name}},
		{{.ValueType}},
		{{.Bytes}},
		{{.ErrorCode}});
	{{ end }}
		});
	});


}
`))

type tmplInput struct {
	SuccessCases       []successCase
	EncodeFailureCases []encodeFailureCase
	DecodeFailureCases []decodeFailureCase
}

type successCase struct {
	Name, Value, ValueType, Bytes string
}

type encodeFailureCase struct {
	Name, Value, ValueType, ErrorCode string
}

type decodeFailureCase struct {
	Name, ValueType, Bytes, ErrorCode string
}

// Generate generates dart tests.
func Generate(wr io.Writer, gidl gidlir.All, fidl fidlir.Root) error {
	successCases, err := successCases(gidl.Success, fidl)
	if err != nil {
		return err
	}
	encodeFailureCases, err := encodeFailureCases(gidl.FailsToEncode, fidl)
	if err != nil {
		return err
	}
	decodeFailureCases := decodeFailureCases(gidl.FailsToDecode)
	return tmpl.Execute(wr, tmplInput{
		SuccessCases:       successCases,
		EncodeFailureCases: encodeFailureCases,
		DecodeFailureCases: decodeFailureCases,
	})
}

func successCases(gidlSuccesses []gidlir.Success, fidl fidlir.Root) (successCases []successCase, err error) {
	for _, success := range gidlSuccesses {
		decl, err := gidlmixer.ExtractDeclaration(success.Value, fidl)
		if err != nil {
			return nil, fmt.Errorf("success %s: %s", success.Name, err)
		}
		valueStr := visit(success.Value, decl)
		successCases = append(successCases, successCase{
			Name:      singleQuote(success.Name),
			Value:     valueStr,
			ValueType: typeName(decl.(*gidlmixer.StructDecl)),
			Bytes:     bytesBuilder(success.Bytes),
		})
	}
	return
}

func encodeFailureCases(gidlEncodeFailures []gidlir.FailsToEncode, fidl fidlir.Root) (encodeFailureCases []encodeFailureCase, err error) {
	for _, encodeFailure := range gidlEncodeFailures {
		decl, err := gidlmixer.ExtractDeclarationUnsafe(encodeFailure.Value, fidl)
		if err != nil {
			return nil, fmt.Errorf("encode failure %s: %s", encodeFailure.Name, err)
		}
		valueStr := visit(encodeFailure.Value, decl)
		encodeFailureCases = append(encodeFailureCases, encodeFailureCase{
			Name:      singleQuote(encodeFailure.Name),
			Value:     valueStr,
			ValueType: typeName(decl.(*gidlmixer.StructDecl)),
			ErrorCode: dartErrorCode(encodeFailure.Err),
		})
	}
	return
}

func decodeFailureCases(gidlDecodeFailures []gidlir.FailsToDecode) (decodeFailureCases []decodeFailureCase) {
	for _, decodeFailure := range gidlDecodeFailures {
		decodeFailureCases = append(decodeFailureCases, decodeFailureCase{
			Name:      singleQuote(decodeFailure.Name),
			ValueType: dartTypeName(decodeFailure.Type),
			Bytes:     bytesBuilder(decodeFailure.Bytes),
			ErrorCode: dartErrorCode(decodeFailure.Err),
		})
	}
	return
}

func typeName(decl *gidlmixer.StructDecl) string {
	parts := strings.Split(string(decl.Name), "/")
	lastPart := parts[len(parts)-1]
	return dartTypeName(lastPart)
}

func dartTypeName(inputType string) string {
	return fmt.Sprintf("k%s_Type", inputType)
}

func dartErrorCode(code string) string {
	return fmt.Sprintf("fidl.FidlErrorCode.%s", snakeCaseToLowerCamelCase(code))
}

func bytesBuilder(bytes []byte) string {
	var sb strings.Builder
	sb.WriteString("Uint8List.fromList([\n")
	for i, b := range bytes {
		sb.WriteString(fmt.Sprintf("0x%02x", b))
		sb.WriteString(",")
		if i%8 == 7 {
			// Note: empty comments are written to preserve formatting. See:
			// https://github.com/dart-lang/dart_style/wiki/FAQ#why-does-the-formatter-mess-up-my-collection-literals
			sb.WriteString(" //\n")
		}
	}
	sb.WriteString("])")
	return sb.String()
}

func visit(value interface{}, decl gidlmixer.Declaration) string {
	switch value := value.(type) {
	case bool:
		return strconv.FormatBool(value)
	case int64:
		return fmt.Sprintf("0x%x", value)
	case uint64:
		return fmt.Sprintf("0x%x", value)
	case string:
		return singleQuote(value)
	case gidlir.Object:
		switch decl := decl.(type) {
		case *gidlmixer.StructDecl:
			return onObject(value, decl)
		case *gidlmixer.TableDecl:
			return onObject(value, decl)
		case *gidlmixer.UnionDecl:
			return onUnion(value, decl)
		case *gidlmixer.XUnionDecl:
			return onUnion(value, decl)
		}
	}
	panic(fmt.Sprintf("unexpected value visited %v (decl %v)", value, decl))
}

func onObject(value gidlir.Object, decl gidlmixer.Declaration) string {
	var args []string
	for _, field := range value.Fields {
		fieldDecl, _ := decl.ForKey(field.Name)
		val := visit(field.Value, fieldDecl)
		args = append(args, fmt.Sprintf("%s: %s", snakeCaseToLowerCamelCase(field.Name), val))
	}
	return fmt.Sprintf("%s(%s)", value.Name, strings.Join(args, ", "))
}

func onUnion(value gidlir.Object, decl gidlmixer.Declaration) string {
	for _, field := range value.Fields {
		fieldDecl, _ := decl.ForKey(field.Name)
		val := visit(field.Value, fieldDecl)
		return fmt.Sprintf("%s.with%s(%s)", value.Name, strings.Title(field.Name), val)
	}
	panic("unions must have a value set")
}

func snakeCaseToLowerCamelCase(snakeString string) string {
	parts := strings.Split(snakeString, "_")
	parts[0] = strings.ToLower(parts[0])
	for i := 1; i < len(parts); i++ {
		parts[i] = upperFirstRune(strings.ToLower(parts[i]))
	}
	return strings.Join(parts, "")
}

func upperFirstRune(s string) string {
	for _, r := range s {
		return strings.ToUpper(string(r)) + s[len(string(r)):]
	}
	return ""
}

func singleQuote(s string) string {
	s = strings.ReplaceAll(s, `\`, `\\`)
	s = strings.ReplaceAll(s, `'`, `\'`)
	return fmt.Sprintf("'%s'", s)
}
