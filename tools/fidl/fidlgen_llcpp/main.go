// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package main

import (
	"flag"
	"fmt"
	"log"
	"os"
	"path/filepath"
	"strings"

	"go.fuchsia.dev/fuchsia/tools/fidl/fidlgen_llcpp/codegen"
	fidl "go.fuchsia.dev/fuchsia/tools/fidl/lib/fidlgen"
	cpp "go.fuchsia.dev/fuchsia/tools/fidl/lib/fidlgen_cpp"
)

type flagsDef struct {
	jsonPath        *string
	header          *string
	source          *string
	testBase        *string
	includeBase     *string
	includeStem     *string
	clangFormatPath *string
}

var flags = flagsDef{
	jsonPath: flag.String("json", "",
		"relative path to the FIDL intermediate representation."),
	header: flag.String("header", "",
		"the output path for the generated header."),
	source: flag.String("source", "",
		"the output path for the generated C++ implementation."),
	testBase: flag.String("test-base", "",
		"the output path for the generated test base header."),
	includeBase: flag.String("include-base", "",
		"[optional] the directory relative to which includes will be computed. "+
			"If omitted, assumes #include <fidl/library/name/llcpp/fidl.h>"),
	includeStem: flag.String("include-stem", "llcpp/fidl",
		"[optional] the suffix after library path when referencing includes. "+
			"Includes will be of the form <my/library/{include-stem}.h>. "),
	clangFormatPath: flag.String("clang-format-path", "",
		"path to the clang-format tool."),
}

// valid returns true if the parsed flags are valid.
func (f flagsDef) valid() bool {
	return *f.jsonPath != "" && *f.header != "" && *f.source != "" && *f.testBase != ""
}

func calcPrimaryHeader(fidl fidl.Root, headerPath string, includeStem string) (string, error) {
	if *flags.includeBase != "" {
		absoluteIncludeBase, err := filepath.Abs(*flags.includeBase)
		if err != nil {
			return "", err
		}
		if !strings.HasPrefix(headerPath, absoluteIncludeBase) {
			return "", fmt.Errorf("include-base (%v) is not a parent of header (%v)", absoluteIncludeBase, headerPath)
		}
		relStem, err := filepath.Rel(*flags.includeBase, *flags.header)
		if err != nil {
			return "", err
		}
		return relStem, nil
	}

	// Assume the convention for including fidl library dependencies, i.e.
	// #include <fuchsia/library/name/llcpp/fidl.h>
	var parts []string
	for _, part := range fidl.Name.Parts() {
		parts = append(parts, string(part))
	}
	return fmt.Sprintf("%s/%s.h", filepath.Join(parts...), includeStem), nil
}

func main() {
	flag.Parse()
	if !flag.Parsed() || !flags.valid() {
		flag.PrintDefaults()
		os.Exit(1)
	}

	fidl, err := fidl.ReadJSONIr(*flags.jsonPath)
	if err != nil {
		log.Fatal(err)
	}
	tree := cpp.CompileLL(fidl)

	headerPath, err := filepath.Abs(*flags.header)
	if err != nil {
		log.Fatal(err)
	}

	sourcePath, err := filepath.Abs(*flags.source)
	if err != nil {
		log.Fatal(err)
	}

	testBasePath, err := filepath.Abs(*flags.testBase)
	if err != nil {
		log.Fatal(err)
	}

	primaryHeader, err := calcPrimaryHeader(fidl, headerPath, *flags.includeStem)
	if err != nil {
		log.Fatal(err)
	}
	tree.PrimaryHeader = primaryHeader
	tree.IncludeStem = *flags.includeStem

	generator := codegen.NewGenerator()
	if err := generator.GenerateHeader(tree, headerPath, *flags.clangFormatPath); err != nil {
		log.Fatalf("Error running header generator: %s", err)
	}
	if err := generator.GenerateSource(tree, sourcePath, *flags.clangFormatPath); err != nil {
		log.Fatalf("Error running source generator: %s", err)
	}
	if err := generator.GenerateTestBase(tree, testBasePath, *flags.clangFormatPath); err != nil {
		log.Fatalf("Error running test base generator: %s", err)
	}
}
