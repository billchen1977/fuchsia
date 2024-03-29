// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package fuzz

import (
	"bufio"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"path/filepath"
	"regexp"
	"runtime"
	"strings"

	"github.com/golang/glog"
)

// A Build represents a Fuchsia build, consisting of all the resources needed
// to run a fuzzer on an instance (e.g. a Fuchsia image, fuzzer packages and
// metadata, binary symbols, support utilities, etc.).
type Build interface {
	// Ensures all the needed resources are present and fetches any that are
	// missing. Multiple calls to Prepare should be idempotent for the same
	// Build.
	Prepare() error

	// Returns a fuzzer specified by a `package/binary` name, or an error if it
	// isn't found.
	Fuzzer(name string) (*Fuzzer, error)

	// Returns the absolute host paths for each key.  Each key corresponds to a
	// specific resource provided by the Build.  This abstraction allows for
	// different build types to have different structures.
	Path(keys ...string) ([]string, error)

	// Reads input from `in`, symbolizes it, and writes it back to `out`.
	// Returns on error, or when `in` has no more data to read.  Processing
	// will be streamed, line-by-line.
	// TODO(fxbug.dev/47482): does this belong elsewhere?
	Symbolize(in io.Reader, out io.Writer) error

	// Returns a list of the names of the fuzzers that are available to run
	ListFuzzers() []string
}

// BaseBuild is a simple implementation of the Build interface
type BaseBuild struct {
	Fuzzers map[string]*Fuzzer
	Paths   map[string]string
	IDs     []string
}

// This is stubbed out to allow for test code to replace it
var NewBuild = NewBuildFromEnvironment

// This environment variable is set by the ClusterFuzz build manager
const clusterFuzzBundleDirEnvVar = "FUCHSIA_RESOURCES_DIR"

// Attempt to auto-detect the correct Build type
func NewBuildFromEnvironment() (Build, error) {
	if _, found := os.LookupEnv(clusterFuzzBundleDirEnvVar); found {
		return NewClusterFuzzLegacyBuild()
	}
	return NewLocalFuchsiaBuild()
}

// NewClusterFuzzLegacyBuild will create a BaseBuild with path layouts
// corresponding to the legacy build bundles used by ClusterFuzz's original
// Python integration. Note that these build bundles only support x64.
func NewClusterFuzzLegacyBuild() (Build, error) {
	bundleDir, found := os.LookupEnv(clusterFuzzBundleDirEnvVar)
	if !found {
		return nil, fmt.Errorf("%s not set", clusterFuzzBundleDirEnvVar)
	}

	buildDir := filepath.Join(bundleDir, "build")
	targetDir := filepath.Join(bundleDir, "target", "x64")
	clangDir := filepath.Join(buildDir, "buildtools", "linux-x64", "clang")
	build := &BaseBuild{
		Paths: map[string]string{
			"zbi":             filepath.Join(targetDir, "fuchsia.zbi"),
			"fvm":             filepath.Join(buildDir, "out", "default", "host_x64", "fvm"),
			"zbitool":         filepath.Join(buildDir, "out", "default", "host_x64", "zbi"),
			"blk":             filepath.Join(targetDir, "fvm.blk"),
			"qemu":            filepath.Join(bundleDir, "qemu-for-fuchsia", "bin", "qemu-system-x86_64"),
			"kernel":          filepath.Join(targetDir, "multiboot.bin"),
			"symbolize":       filepath.Join(buildDir, "zircon", "prebuilt", "downloads", "symbolize", "linux-x64", "symbolize"),
			"llvm-symbolizer": filepath.Join(clangDir, "bin", "llvm-symbolizer"),
			"fuzzers.json":    filepath.Join(buildDir, "out", "default", "fuzzers.json"),
			"authkeys":        filepath.Join(bundleDir, ".ssh", "authorized_keys"),
			"sshid":           filepath.Join(bundleDir, ".ssh", "pkey"),
		},
		IDs: []string{
			filepath.Join(clangDir, "lib", "debug", ".build_id"),
			filepath.Join(buildDir, "out", "default", ".build-id"),
		},
	}
	if err := build.LoadFuzzers(); err != nil {
		return nil, err
	}

	return build, nil
}

var Platforms = map[string]string{
	"linux":  "linux",
	"darwin": "mac",
}

var Archs = map[string]struct {
	Binary string
	Kernel string
}{
	"x64":   {"qemu-system-x86_64", "multiboot.bin"},
	"arm64": {"qemu-system-aarch64", "qemu-boot-shim.bin"},
}

var hostDir = map[string]string{"arm64": "host_arm64", "amd64": "host_x64"}[runtime.GOARCH]

// NewLocalFuchsiaBuild will create a BaseBuild with path layouts corresponding
// to a local Fuchsia checkout
func NewLocalFuchsiaBuild() (Build, error) {
	fuchsiaDir := os.Getenv("FUCHSIA_DIR")
	if fuchsiaDir == "" {
		// Fall back to relative path from this file
		fuchsiaDir = filepath.Join("..", "..")
	}

	fxBuildDir := filepath.Join(fuchsiaDir, ".fx-build-dir")
	contents, err := ioutil.ReadFile(fxBuildDir)
	if err != nil {
		return nil, fmt.Errorf("failed to read %q: %s", fxBuildDir, err)
	}

	buildDir := filepath.Join(fuchsiaDir, strings.TrimSpace(string(contents)))
	zirconBuildDir := buildDir + ".zircon"
	prebuiltDir := filepath.Join(fuchsiaDir, "prebuilt")

	platform, ok := Platforms[runtime.GOOS]
	if !ok {
		return nil, fmt.Errorf("unsupported os: %s", runtime.GOOS)
	}

	fxConfig := filepath.Join(buildDir, "fx.config")
	file, err := os.Open(fxConfig)
	if err != nil {
		return nil, fmt.Errorf("failed to open %q: %s", fxConfig, err)
	}
	defer file.Close()

	properties := map[string]string{}
	re := regexp.MustCompile(`^([^=]+)=(?:'([^']+)'|(.+))?$`)
	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		m := re.FindStringSubmatch(scanner.Text())
		if m != nil {
			properties[m[1]] = m[2]
		}
	}

	arch, found := properties["FUCHSIA_ARCH"]
	if !found {
		return nil, fmt.Errorf("no arch in %s", fxConfig)
	}

	archInfo, ok := Archs[arch]
	if !ok {
		supported := make([]string, 0, len(Archs))
		for k := range Archs {
			supported = append(supported, k)
		}
		return nil, fmt.Errorf("unsupported arch: %s (supported: %v)", arch, supported)
	}

	binary := archInfo.Binary
	kernel := archInfo.Kernel
	platform += "-" + arch

	clangDir := filepath.Join(prebuiltDir, "third_party/clang", platform)
	qemuDir := filepath.Join(prebuiltDir, "third_party/qemu", platform)

	f, err := os.Open(filepath.Join(fuchsiaDir, ".fx-ssh-path"))
	if err != nil {
		return nil, fmt.Errorf("wanted SSH manifest, couldn't open: %v", err)
	}
	defer f.Close()
	s := bufio.NewScanner(f)
	if !s.Scan() {
		// File format must have two lines.
		return nil, fmt.Errorf("expected 2 lines in .fx-ssh-path, found 0")
	}
	if err := s.Err(); err != nil {
		return nil, fmt.Errorf("error reading SSH key paths: %v", err)
	}

	sshid := s.Text()
	if !s.Scan() {
		// File format must have two lines.
		return nil, fmt.Errorf("expected 2 lines in .fx-ssh-path, found 1")
	}
	if err := s.Err(); err != nil {
		return nil, fmt.Errorf("error reading SSH key paths: %v", err)
	}
	authkeys := s.Text()

	build := &BaseBuild{
		Paths: map[string]string{
			"zbi":             filepath.Join(buildDir, "fuchsia.zbi"),
			"fvm":             filepath.Join(buildDir, hostDir, "fvm"),
			"zbitool":         filepath.Join(zirconBuildDir, "tools", "zbi"),
			"blk":             filepath.Join(buildDir, "obj", "build", "images", "fvm.blk"),
			"qemu":            filepath.Join(qemuDir, "bin", binary),
			"kernel":          filepath.Join(zirconBuildDir, kernel),
			"symbolize":       filepath.Join(buildDir, hostDir, "symbolize"),
			"llvm-symbolizer": filepath.Join(clangDir, "bin", "llvm-symbolizer"),
			"fuzzers.json":    filepath.Join(buildDir, "fuzzers.json"),
			"authkeys":        authkeys,
			"sshid":           sshid,
		},
		IDs: []string{
			filepath.Join(clangDir, "lib", "debug", ".build-id"),
			filepath.Join(buildDir, ".build-id"),
			filepath.Join(zirconBuildDir, ".build-id"),
		},
	}
	if err := build.LoadFuzzers(); err != nil {
		return nil, err
	}

	return build, nil
}

// Convenience type alias for heterogenous metadata objects in fuzzers.json
type fuzzerMetadata map[string]string

// LoadFuzzers reads and parses fuzzers.json to populate the build's map of Fuzzers.
// Unless an error is returned, any previously loaded fuzzers will be discarded.
func (b *BaseBuild) LoadFuzzers() error {
	paths, err := b.Path("fuzzers.json")
	if err != nil {
		return err
	}

	jsonPath := paths[0]

	glog.Infof("Loading fuzzers from %q", jsonPath)

	jsonBlob, err := ioutil.ReadFile(jsonPath)
	if err != nil {
		return fmt.Errorf("failed to read %q: %s", jsonPath, err)
	}

	var metadataList []fuzzerMetadata
	if err := json.Unmarshal(jsonBlob, &metadataList); err != nil {
		return fmt.Errorf("failed to parse %q: %s", jsonPath, err)
	}

	// Condense metadata entries by label
	metadataByLabel := make(map[string]fuzzerMetadata)
	for _, metadata := range metadataList {
		label, found := metadata["label"]
		if !found {
			return fmt.Errorf("failed to parse %q: entry missing label", jsonPath)
		}

		if _, found := metadataByLabel[label]; !found {
			metadataByLabel[label] = make(fuzzerMetadata)
		}

		for k, v := range metadata {
			if v != "" {
				metadataByLabel[label][k] = v
			}
		}
	}

	b.Fuzzers = make(map[string]*Fuzzer)
	for label, metadata := range metadataByLabel {
		pkg, found := metadata["package"]
		if !found {
			return fmt.Errorf("failed to parse %q: no package for %q", jsonPath, label)
		}

		fuzzer, found := metadata["fuzzer"]
		if !found {
			return fmt.Errorf("failed to parse %q: no fuzzer for %q", jsonPath, label)
		}

		f := NewFuzzer(b, pkg, fuzzer)
		b.Fuzzers[f.Name] = f
	}

	return nil
}

// ListFuzzers lists the names of fuzzers present in the build
// TODO(fxbug.dev/45108): handle variant stripping
func (b *BaseBuild) ListFuzzers() []string {
	var names []string
	for k := range b.Fuzzers {
		names = append(names, k)
	}
	return names
}

// Fuzzer finds the Fuzzer with the given name, if available
func (b *BaseBuild) Fuzzer(name string) (*Fuzzer, error) {
	fuzzer, found := b.Fuzzers[name]
	if !found {
		return nil, fmt.Errorf("no such fuzzer: %s", name)
	}
	return fuzzer, nil
}

// Prepare is a no-op for simple builds
func (b *BaseBuild) Prepare() error {
	return nil
}

// Path returns the absolute paths to the list of files indicated by keys. This
// allows callers to abstract away the detail of where specific file resources
// are.
func (b *BaseBuild) Path(keys ...string) ([]string, error) {
	paths := make([]string, len(keys))
	for i, key := range keys {
		if path, found := b.Paths[key]; found {
			paths[i] = path
		} else {
			return nil, fmt.Errorf("no path for %q", key)
		}
	}
	return paths, nil
}

var logPrefixRegex = regexp.MustCompile(`[0-9\[\]\.]*\[klog\] INFO: `)

// Remove timestamps, etc.
func stripLogPrefix(line string) string {
	return logPrefixRegex.ReplaceAllString(line, "")
}

// Symbolize reads from in and replaces symbolizer markup with debug
// information before writing the result to out.  This is blocking, and does
// not propagate EOFs from in to out.
func (b *BaseBuild) Symbolize(in io.Reader, out io.Writer) error {
	paths, err := b.Path("symbolize", "llvm-symbolizer")
	if err != nil {
		return err
	}
	symbolize, llvmSymbolizer := paths[0], paths[1]

	args := []string{"-llvm-symbolizer", llvmSymbolizer}
	for _, dir := range b.IDs {
		args = append(args, "-build-id-dir", dir)
	}
	cmd := NewCommand(symbolize, args...)
	cmd.Stdin = in
	pipe, err := cmd.StdoutPipe()
	if err != nil {
		return err
	}
	if err := cmd.Start(); err != nil {
		return err
	}
	scanner := bufio.NewScanner(pipe)

	for scanner.Scan() {
		io.WriteString(out, stripLogPrefix(scanner.Text())+"\n")
	}

	if err := scanner.Err(); err != nil {
		return fmt.Errorf("failed during scan: %s", err)
	}

	return cmd.Wait()
}
