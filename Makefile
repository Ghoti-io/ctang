SUITE := ghoti.io
PROJECT := tang

BUILD ?= release
# The version of this library. MINOR_VERSION carries the minor and the patch as
# one dotted string; the two are split out below for the places that need three
# separate integers. See CONVENTIONS.md section 4.
MAJOR_VERSION := 0
MINOR_VERSION := 0.0
VERSION_MINOR_ONLY := $(word 1,$(subst ., ,$(MINOR_VERSION)))
VERSION_PATCH_ONLY := $(or $(word 2,$(subst ., ,$(MINOR_VERSION))),0)
# Substituted into the .pc file; an empty Version: field makes every
# pkg-config version constraint fail.
VERSION := $(MAJOR_VERSION).$(MINOR_VERSION)

# Names this build everywhere: the .pc file, the install directory, the soname
# and the symbol token. It defaults to the major version, so an ordinary build
# of 1.x is "-1" and two majors cannot be loaded into one process by mistake.
# Override it for a build that wants its own identity:  make BRANCH=-dev
BRANCH ?= -$(MAJOR_VERSION)

# What the library reports as its version. The branch is appended only when it
# is not the default, so an ordinary build says "1.2.3" and an overridden one
# says "1.2.3-dev". Computed before BUILD=debug rewrites BRANCH below.
ifeq ($(BRANCH),-$(MAJOR_VERSION))
VERSION_STRING := $(VERSION)
else
VERSION_STRING := $(VERSION)$(BRANCH)
endif

# If BUILD is debug, append -debug.
#
# "override" because BRANCH may have come from the command line, and a
# command-line variable otherwise wins over a plain assignment here: without it
# `make BRANCH=-dev BUILD=debug` produced a debug build carrying the release
# token, whose symbols collide with the release build's.
ifeq ($(BUILD),debug)
    override BRANCH := $(BRANCH)-debug
    override VERSION_STRING := $(VERSION_STRING)-debug
endif

BASE_NAME := lib$(SUITE)-$(PROJECT)$(BRANCH).so
# The symbol namespace token, from BRANCH. See CONVENTIONS.md section 4.
LIBVER_SYMBOL := $(shell echo "ghotiio_$(PROJECT)$(BRANCH)" | sed 's/[.-]/_/g')

BASE_NAME_PREFIX := lib$(SUITE)-$(PROJECT)$(BRANCH)
STATIC_TARGET := $(BASE_NAME_PREFIX).a
SO_NAME := $(BASE_NAME).$(MAJOR_VERSION)

# PC_INSTALL_PATH names where this project's own .pc file is installed.
# PKG_CONFIG_PATH is the environment's and is never assigned here: make exports
# an inherited variable with whatever value the makefile last gave it, so
# overwriting it handed every sub-make a different PKG_CONFIG_PATH from the
# parent's. The sub-make then derived different flags, found the flag stamp
# changed, and rebuilt everything - which check-rebuild reports as a settled
# tree that will not settle. It showed first under MSYS2, whose login shell
# exports PKG_CONFIG_PATH, and happens on Linux whenever the exported value is
# not exactly the install location. cutil made the same change.
PKG_CONFIG_PATH_ENV := $(PKG_CONFIG_PATH)

# `override` on each of those: BUILD may arrive on the command line, and a
# command-line variable beats a plain makefile assignment, so without it
# `make BUILD=debug` skips the rewrite and builds into ./build/debug --
# outside the platform tree, and a different tree from the one plain `make`
# uses. The platform segment exists to keep linux/mac/win builds apart.

# Detect OS
#
# OS_SPECIFIC_OBJECT_FLAGS holds what a *compile* needs, because the three
# recipes that use it all compile with -c and nothing links with it. It was
# spelled `-shared -fPIC` (and `-shared` alone on the other three platforms)
# until 2026-09-23, and the -shared half had never done anything: it is a
# linker flag, and the shared-library rule spells its own -shared literally.
# gcc ignores unused arguments, so nothing here ever said so; clang makes it
# `argument unused during compilation` and, under ctang's -Werror, refuses to
# build the library at all. Dropping it changes no gcc build on any platform.
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S), Linux)
	OS_NAME := Linux
	LIB_EXTENSION := so
	OS_SPECIFIC_OBJECT_FLAGS := -fPIC
	OS_SPECIFIC_LIBRARY_NAME_FLAG := -Wl,-soname,$(SO_NAME)
	TARGET := $(SO_NAME).$(MINOR_VERSION)
	EXE_EXTENSION :=
	# Additional Linux-specific variables
	PC_INSTALL_PATH := /usr/local/share/pkgconfig
	INCLUDE_INSTALL_PATH := /usr/local/include
	LIB_INSTALL_PATH := /usr/local/lib
	override BUILD := linux/$(BUILD)

else ifeq ($(UNAME_S), Darwin)
	OS_NAME := Mac
	LIB_EXTENSION := dylib
	OS_SPECIFIC_OBJECT_FLAGS :=
	OS_SPECIFIC_LIBRARY_NAME_FLAG := -Wl,-install_name,$(BASE_NAME_PREFIX).dylib
	TARGET := $(BASE_NAME_PREFIX).dylib
	EXE_EXTENSION :=
	# Additional macOS-specific variables
	override BUILD := mac/$(BUILD)

else ifeq ($(findstring MINGW32_NT,$(UNAME_S)),MINGW32_NT)  # 32-bit Windows
	OS_NAME := Windows
	LIB_EXTENSION := dll
	OS_SPECIFIC_OBJECT_FLAGS :=
	OS_SPECIFIC_LIBRARY_NAME_FLAG = -Wl,--out-implib,$(APP_DIR)/$(BASE_NAME_PREFIX).dll.a
	TARGET := $(BASE_NAME_PREFIX).dll
	EXE_EXTENSION := .exe
	# Additional Windows-specific variables
	# This is the path to the pkg-config files on MSYS2
	PC_INSTALL_PATH := /mingw32/lib/pkgconfig
	INCLUDE_INSTALL_PATH := /mingw32/include
	LIB_INSTALL_PATH := /mingw32/lib
	BIN_INSTALL_PATH := /mingw32/bin
	override BUILD := win32/$(BUILD)

else ifeq ($(findstring MINGW64_NT,$(UNAME_S)),MINGW64_NT)  # 64-bit Windows
	OS_NAME := Windows
	LIB_EXTENSION := dll
	OS_SPECIFIC_OBJECT_FLAGS :=
	OS_SPECIFIC_LIBRARY_NAME_FLAG = -Wl,--out-implib,$(APP_DIR)/$(BASE_NAME_PREFIX).dll.a
	TARGET := $(BASE_NAME_PREFIX).dll
	EXE_EXTENSION := .exe
	# Additional Windows-specific variables
	# This is the path to the pkg-config files on MSYS2
	PC_INSTALL_PATH := /mingw64/lib/pkgconfig
	INCLUDE_INSTALL_PATH := /mingw64/include
	LIB_INSTALL_PATH := /mingw64/lib
	BIN_INSTALL_PATH := /mingw64/bin
	override BUILD := win64/$(BUILD)

else
    $(error Unsupported OS: $(UNAME_S))

endif

# ---------------------------------------------------------------------------
# Installation prefix
#
# Defaults to the system location chosen above. Override it to install
# somewhere else - the suite's bootstrap installs every library into a local
# prefix so that each build resolves its dependencies through pkg-config,
# exactly as a consumer would, rather than through a second code path that
# only in-tree builds exercise. See CONVENTIONS.md section 1.
#
#     make install PREFIX=/path/to/prefix
# ---------------------------------------------------------------------------
ifdef PREFIX
INCLUDE_INSTALL_PATH := $(PREFIX)/include
LIB_INSTALL_PATH := $(PREFIX)/lib
BIN_INSTALL_PATH := $(PREFIX)/bin
PC_INSTALL_PATH := $(PREFIX)/share/pkgconfig
ifeq ($(OS_NAME), Windows)
PC_INCLUDE_DIR = $(shell cygpath -m $(INCLUDE_INSTALL_PATH)/$(SUITE)/$(PROJECT)$(BRANCH))
PC_LIB_DIR = $(shell cygpath -m $(LIB_INSTALL_PATH)/$(SUITE))
else
PC_INCLUDE_DIR := $(INCLUDE_INSTALL_PATH)/$(SUITE)/$(PROJECT)$(BRANCH)
PC_LIB_DIR := $(LIB_INSTALL_PATH)/$(SUITE)
endif
# A non-system prefix has no /etc/ld.so.conf.d, and writing to it would need
# root anyway. Everything built here carries an rpath to the prefix instead.
LDCONF_INSTALL_PATH :=
endif

# Dependencies are looked up along the inherited PKG_CONFIG_PATH as well as the
# install location chosen above, so that exporting PKG_CONFIG_PATH works as the
# errors below say it does. The inherited value comes first: it is an explicit
# request for this build, where the install location may be only a default.
PKG_CONFIG_LOOKUP_PATH := $(if $(PKG_CONFIG_PATH_ENV),$(PKG_CONFIG_PATH_ENV):)$(PC_INSTALL_PATH)


# The optimization level is the one thing that distinguishes the two builds'
# compile flags. `release` is what gets installed and what anything linking
# against this library actually runs, so it is compiled for speed; `debug` is
# compiled for stepping through. -g stays in both, because a release build that
# cannot be read in a debugger is a release build nobody can diagnose, and a JIT
# is worth having readable backtraces for; the symbols cost only file size, not
# run time, since -g emits DWARF rather than different code.
#
# BUILD=debug used to change only the artifact's *name* - it appended -debug to
# BRANCH and VERSION_STRING and nothing else - so `make BUILD=debug` produced a
# -O2 binary wearing a debug filename, which is the one build you cannot step
# through.
#
# Where the other -O levels in this file come from, since last-one-wins only
# helps if the flags actually arrive in that order:
#   - `make coverage` appends --coverage -O0 through EXTRA_CFLAGS, which is at
#     the end of CFLAGS, so its -O0 wins.
#   - The fuzz build does not use CFLAGS at all; FUZZ_LIB_FLAGS carries its own
#     -O1.
#   - The sanitizer build is the exception worth knowing about: SAN_FLAGS has no
#     -O of its own, and SAN_CFLAGS is $(CFLAGS) $(SAN_FLAGS), so `make
#     test-asan` runs at whatever OPT_CFLAGS says rather than at a level of its
#     own. That is why it follows BUILD along with everything else. Measured:
#
#       62 C   translation units at -O2   (the library, inheriting release)
#        7 C++ translation units at -O1   (the test binaries)
#
#     Seven of the nine libraries in the suite are shaped this way, so the
#     sanitizer gate's fidelity moves with the release level rather than being
#     pinned.
#
#     SETTLED for ctang on 2026-09-23: it stays inheriting. A sanitizer gate
#     built at a different -O than the shipped library is testing a different
#     program, so matching is the option that needs no argument and diverging
#     is the one that does. Inheriting also keeps `BUILD=debug` able to put the
#     gate at -O0 when a stack trace matters, which pinning would take away.
#
#     The argument that used to weigh against this was aliasing coverage. It
#     never worked, for a reason better than the one first written here: no
#     sanitizer sees this class at any -O. See below. This is ctang's decision
#     on ctang's evidence, not a suite-wide ruling; the other eight still have
#     to make it for themselves.
#
#     One cost of pinning that is easy to miss, measured here on gcc 14.2.0
#     rather than taken on report: -Wstrict-aliasing only fires when
#     -fstrict-aliasing is active, and gcc enables that at -O2, not before.
#     `gcc -O1 -Q --help=optimizers` prints [disabled] for it and -O2 prints
#     [enabled]. On a type-punning probe, -O1 alone found nothing, adding
#     -fstrict-aliasing to -O1 found it, and -O2 -fno-strict-aliasing found
#     nothing - the warning follows the flag rather than the level. So pinning
#     the sanitizer build at -O1 drops aliasing checking silently unless
#     -fstrict-aliasing is named alongside it.
#
#     The paragraph above is RIGHT and an earlier version of this block
#     briefly said otherwise. Level sensitivity is a property of the
#     construct, along two axes measured pairwise on gcc 14.2.0 at -O2:
#
#       *(int *)&g        address of a visible object, in place    1 2 3
#       int * p = ...&g   same object, via a pointer variable      1 2
#       *(int *)d         d is a parameter                         1
#       struct-to-struct  cast of a parameter                      1
#       through a void *                                           none
#
#     Taking the address of an object the compiler can see is what level 2
#     needs; routing the cast through a separate pointer variable is what
#     defeats level 3, which reports the in-place dereference and nothing
#     else. Level 1 dominates - it catches whatever any level catches.
#
#     So ctang's effective level 3 is a narrow instrument, not an absent one:
#     it fires on the first row under ctang's real flags and on none of the
#     rest. Four constructs probed here at first all sat on the same side of
#     both axes, which produced a clean and wrong "level 3 catches nothing".
#
#     Level 1 cannot be adopted regardless: building the library with it emits
#     669 diagnostics across 48 of the 62 translation units, every one of them
#     the C struct-inheritance downcast `(GTA_Ast_Node_Boolean *) self`, which
#     C17 6.7.2.1p15 makes well defined. That idiom IS the vtable mechanism
#     this library is built from, so they are not a backlog to work through.
#
#     None of which changes the decision above, because the aliasing argument
#     for matching the shipped -O never worked anyway: NO sanitizer detects
#     this class at ANY -O, so "the optimizer exploits aliasing at -O2,
#     therefore run the gate at -O2" is a true claim about the optimizer
#     welded to a false one about the gate. The gate stays inherited for the
#     reasons given above, not for this one.
#
#     And a green `make test CC=clang` is not evidence here either way: clang
#     accepts -Wstrict-aliasing=1 and =2, implements neither, and rejects =3.
#
#     Those figures are read out of the ARTIFACTS, not parsed out of `make -n`.
#     gcc records the command line in DWARF by default (-grecord-gcc-switches),
#     so the flags an object was actually built with are in the object:
#
#       readelf --debug-dump=info <file> | grep DW_AT_producer
#
#     and the level is the last -O in that string. The 7 test units have no .o
#     of their own - each test binary is compiled and linked in one step - so
#     for those, read the executable and keep the C++ compile units. Exclude
#     gcc's own libsanitizer CUs, which are in there too (asan_preinit.cpp and
#     friends, at -O2) and are not ours.
#
#     Four successive attempts to get this number by parsing `make -n` output
#     were each wrong in a different direction, so the text is not the
#     instrument. Recorded so nobody repeats them: counting every -O on a line
#     rather than the last invents compilations, since only the last is obeyed.
#     Selecting lines by ` -c ` drops every unit compiled and linked in one
#     step - here all 7 C++ units - and reports 62 as though it were the whole
#     gate, while the self-check still sums, because the dropped lines are
#     missing from both sides. Selecting instead on the line naming a source
#     over-matches in other libraries of the suite, roughly doubling their
#     counts, for a reason nobody has yet traced; it happens to be right here,
#     which is not a recommendation.
ifeq ($(BUILD),debug)
OPT_CFLAGS := -O0
OPT_CXXFLAGS := -O0
else
# -O2 rather than -O3: measured across five workloads and both execution paths,
# -O3 came out slower than -O2, and -O2 captured essentially the whole win over
# -O0 (6-9%).
#
# Treat that as a measurement of today's ctang rather than a finding about -O3.
# The library still has untraced inefficiencies, and a -O3-is-slower result on a
# codebase with unexplained overhead may be describing the overhead - -O3's
# extra inlining is exactly the kind of thing that could be interacting with it.
# Re-measure once those are traced; until then -O2 stands because it is the
# suite floor, not because -O3 was ruled out.
#
# It is tempting to discount that span on the grounds that a share of ctang's
# run time is JIT-emitted machine code no -O level recompiles. That reasoning
# was pre-registered as a prediction when the levels were measured, and it was
# wrong: fib(30) gained 20% under the JIT and 24% under bytecode, essentially
# the same. perf says why - only 9.85% of a JIT-executed fib(32) is in emitted
# instructions, while 48% is ctang's own C runtime and 35% is libc, because the
# JIT emits a call into that runtime for essentially every arithmetic operation
# and comparison. The JIT is native code calling C per operation, not a program
# running natively, so -O reaches almost all of it.
#
# What the figure really carries is the allocator: a third of fib's time is
# malloc, one computed value heap-allocated per integer result. That is the
# untraced inefficiency above, with a number on it - and it is a far better
# reason to distrust a -O3 comparison than the JIT is, since allocation
# behaviour is exactly what extra inlining perturbs. Fixing value reuse is
# worth more than any -O level here.
OPT_CFLAGS := -O2
# The C++ here is test translation units only - no shipped code - so this is a
# build-time-versus-test-run-time trade, not the policy's subject.
OPT_CXXFLAGS := -O1
endif


CXX := g++
CXXFLAGS := -pedantic-errors -Wall -Wextra -Werror -Wno-error=unused-function -Wfatal-errors -std=c++20 $(OPT_CXXFLAGS) -g $(EXTRA_CXXFLAGS)
CC := cc
# cutil, found through pkg-config. The name carries the branch, which is how a
# consumer picks a version; CUTIL_PC is overridable so this library can be
# built against a cutil on a different branch from its own.
# Targets that neither compile nor link can run without the suite installed.
# The dependency check below is evaluated while this file is read, not when a
# target is considered, so without this guard it fires for every goal - and
# `make clean`, the one thing you reach for when a tree is in a bad state,
# exits non-zero having removed nothing.
#
# $(or $(MAKECMDGOALS),all) is load-bearing: a bare `make` names no goal, and
# must be treated as `all` so that it still gets checked. Reading it as "no
# goals, therefore nothing outside the list" would skip the check in exactly
# the case it exists for.
# uninstall is here because it finds what it removes through PREFIX and asks
# pkg-config nothing: needing the dependencies present in order to REMOVE the
# library is the same defect as needing them in order to clean, and the likely
# reason anyone is uninstalling is that something is already broken.
# uninstall-debug recurses into uninstall, and the outer goal is tested before
# the inner one runs, so both names have to be here.
#
# The `Unsupported OS:` error further up is deliberately NOT guarded this way.
# Each OS branch sets BUILD, BUILD_DIR derives from it, and uninstall's recipe
# switches on OS_NAME - so on an unrecognised OS a guarded error would turn a
# clear message into a clean that removes the wrong tree and an uninstall that
# silently removes nothing. That is this fix's own defect, reintroduced by
# over-applying it.
DEPLESS_GOALS := clean cloc docs docs-pdf help uninstall uninstall-debug fuzz-clean
ifeq ($(filter-out $(DEPLESS_GOALS),$(or $(MAKECMDGOALS),all)),)
SKIP_DEP_CHECK := 1
endif

CUTIL_PC ?= ghoti.io-cutil$(BRANCH)
CUTIL_CFLAGS := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_LOOKUP_PATH) pkg-config --cflags $(CUTIL_PC) 2>/dev/null)
CUTIL_LIBS := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_LOOKUP_PATH) pkg-config --libs $(CUTIL_PC) 2>/dev/null)
ifndef SKIP_DEP_CHECK
ifeq ($(strip $(CUTIL_CFLAGS)),)
$(error ghoti.io-cutil was not found by pkg-config. Run ./bootstrap.sh in the parent folder to build and install the suite into a local prefix, then pass the same PREFIX here - or point PKG_CONFIG_PATH at the directory holding its .pc file. There is deliberately no sibling-checkout fallback: a second resolution path that only in-tree builds exercise is one that silently rots.)
endif
endif
UNICODE_PC ?= ghoti.io-unicode$(BRANCH)
UNICODE_CFLAGS := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_LOOKUP_PATH) pkg-config --cflags $(UNICODE_PC) 2>/dev/null)
UNICODE_LIBS := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_LOOKUP_PATH) pkg-config --libs $(UNICODE_PC) 2>/dev/null)
ifndef SKIP_DEP_CHECK
ifeq ($(strip $(UNICODE_CFLAGS)),)
$(error ghoti.io-unicode was not found by pkg-config. Run ./bootstrap.sh in the parent folder to build and install the suite into a local prefix, then pass the same PREFIX here - or point PKG_CONFIG_PATH at the directory holding its .pc file. There is deliberately no sibling-checkout fallback: a second resolution path that only in-tree builds exercise is one that silently rots.)
endif
endif
CFLAGS := -pedantic-errors -Wall -Wextra -Werror -Wno-error=unused-function -Wfatal-errors -std=c17 $(OPT_CFLAGS) -g $(UNICODE_CFLAGS) $(CUTIL_CFLAGS) $(EXTRA_CFLAGS)

# The shipped library exports its public API and nothing else. Tests reach the
# internals by linking the static archive, which a static link can do even for
# hidden symbols.
ifeq ($(OS_NAME), Windows)
# Everything built here but the library itself links the static archive or
# the library's objects directly, so the headers must not say dllimport to it:
# neither provides __imp_ thunks. The library's own objects also get
# GHOTIIO_TANG_BUILD, which the header tests first. See GTA_API in macros.h.
CFLAGS += -DGHOTIIO_TANG_STATIC
CXXFLAGS += -DGHOTIIO_TANG_STATIC
endif
LIB_CFLAGS := $(CFLAGS) -fvisibility=hidden -DGHOTIIO_TANG_BUILD

# The bison and flex output is not ours to edit - a regeneration would discard
# any fix - and gcc and clang disagree about which parts of it to complain
# about. gcc wants -Wno-unused-function for the scanner; clang additionally
# reports an unused-but-set GTA_Parser_nerrs in the parser and an unneeded
# internal declaration in the scanner. Each compiler silently accepts the
# other's -Wno- spelling, so one list serves both.
GENERATED_CFLAGS := -Wno-unused-function -Wno-unused-but-set-variable \
                    -Wno-unneeded-internal-declaration
# -DGHOTIIO_CUTIL_ENABLE_MEMORY_DEBUG
LDFLAGS := -L /usr/lib -lstdc++ -lm $(UNICODE_LIBS) $(CUTIL_LIBS) $(EXTRA_LDFLAGS)
ifdef PREFIX
# So that a library, a test or an example finds its Ghoti.io dependencies in the
# prefix at run time without LD_LIBRARY_PATH.
LDFLAGS += -Wl,-rpath,$(LIB_INSTALL_PATH)/$(SUITE)
ifeq ($(OS_NAME), Windows)
# Windows has no rpath: a program finds its DLLs through PATH. Putting the
# prefix's bin/ on it for everything make runs is the equivalent, so that a
# test or an example finds its dependencies without the caller arranging it.
# Without this they die before main() with 0xC0000135 and make reports 127.
export PATH := $(BIN_INSTALL_PATH):$(PATH)
endif
endif

# The C++ test translation units include cutil and unicode headers too.
CXXFLAGS += $(UNICODE_CFLAGS) $(CUTIL_CFLAGS)
BUILD_DIR := ./build/$(BUILD)
OBJ_DIR := $(BUILD_DIR)/objects
FLAGS_STAMP := $(OBJ_DIR)/.flags
GEN_DIR := $(BUILD_DIR)/generated
APP_DIR := $(BUILD_DIR)/apps


INCLUDE := -I include/ -I $(GEN_DIR)/
LIBOBJECTS := \
	$(OBJ_DIR)/allocator.o \
	$(OBJ_DIR)/tangParser.o \
	$(OBJ_DIR)/tangScanner.o \
	$(OBJ_DIR)/unicodeString.o \
	$(OBJ_DIR)/ast/astNode.o \
	$(OBJ_DIR)/ast/astNodeArray.o \
	$(OBJ_DIR)/ast/astNodeAssign.o \
	$(OBJ_DIR)/ast/astNodeBinary.o \
	$(OBJ_DIR)/ast/astNodeBlock.o \
	$(OBJ_DIR)/ast/astNodeBoolean.o \
	$(OBJ_DIR)/ast/astNodeBreak.o \
	$(OBJ_DIR)/ast/astNodeCast.o \
	$(OBJ_DIR)/ast/astNodeContinue.o \
	$(OBJ_DIR)/ast/astNodeDoWhile.o \
	$(OBJ_DIR)/ast/astNodeFloat.o \
	$(OBJ_DIR)/ast/astNodeFor.o \
	$(OBJ_DIR)/ast/astNodeFunction.o \
	$(OBJ_DIR)/ast/astNodeFunctionCall.o \
	$(OBJ_DIR)/ast/astNodeGlobal.o \
	$(OBJ_DIR)/ast/astNodeIdentifier.o \
	$(OBJ_DIR)/ast/astNodeIfElse.o \
	$(OBJ_DIR)/ast/astNodeIndex.o \
	$(OBJ_DIR)/ast/astNodeInteger.o \
	$(OBJ_DIR)/ast/astNodeLibrary.o \
	$(OBJ_DIR)/ast/astNodeMap.o \
	$(OBJ_DIR)/ast/astNodePeriod.o \
	$(OBJ_DIR)/ast/astNodeParseError.o \
	$(OBJ_DIR)/ast/astNodePrint.o \
	$(OBJ_DIR)/ast/astNodeRangedFor.o \
	$(OBJ_DIR)/ast/astNodeReturn.o \
	$(OBJ_DIR)/ast/astNodeSlice.o \
	$(OBJ_DIR)/ast/astNodeString.o \
	$(OBJ_DIR)/ast/astNodeTernary.o \
	$(OBJ_DIR)/ast/astNodeUnary.o \
	$(OBJ_DIR)/ast/astNodeUse.o \
	$(OBJ_DIR)/ast/astNodeWhile.o \
	$(OBJ_DIR)/computedValue/computedValue.o \
	$(OBJ_DIR)/computedValue/computedValueArray.o \
	$(OBJ_DIR)/computedValue/computedValueBoolean.o \
	$(OBJ_DIR)/computedValue/computedValueError.o \
	$(OBJ_DIR)/computedValue/computedValueFloat.o \
	$(OBJ_DIR)/computedValue/computedValueFunction.o \
	$(OBJ_DIR)/computedValue/computedValueFunctionNative.o \
	$(OBJ_DIR)/computedValue/computedValueInteger.o \
	$(OBJ_DIR)/computedValue/computedValueIterator.o \
	$(OBJ_DIR)/computedValue/computedValueLibrary.o \
	$(OBJ_DIR)/computedValue/computedValueMap.o \
	$(OBJ_DIR)/computedValue/computedValueRNG.o \
	$(OBJ_DIR)/computedValue/computedValueString.o \
	$(OBJ_DIR)/library/library.o \
	$(OBJ_DIR)/library/libraryMath.o \
	$(OBJ_DIR)/library/libraryRandom.o \
	$(OBJ_DIR)/program/binary.o \
	$(OBJ_DIR)/program/bytecode.o \
	$(OBJ_DIR)/program/compilerContext.o \
	$(OBJ_DIR)/program/executionContext.o \
	$(OBJ_DIR)/program/garbageCollector.o \
	$(OBJ_DIR)/program/language.o \
	$(OBJ_DIR)/program/program.o \
	$(OBJ_DIR)/program/variable.o \
	$(OBJ_DIR)/tangLanguage.o \
	$(OBJ_DIR)/program/virtualMachine.o \


TESTFLAGS := `PKG_CONFIG_PATH=$(PKG_CONFIG_LOOKUP_PATH) pkg-config --libs --cflags gtest`

# The checks `make test` runs besides the tests themselves. Named in a
# variable so that a build which cannot satisfy them can clear it: the
# coverage target does, because --coverage links the gcov runtime, whose
# mangle_path check-symbols is right to reject in a shipping library and
# wrong to reject in an instrumented one. Spelled as text's TEST_GATES is.
TEST_GATES ?= check-symbols



# The static archive, not -l: a static link resolves hidden symbols, so the
# tests can exercise internals the shared library does not export. unicode and
# cutil follow it, because an archive carries no DT_NEEDED of its own.
TANGLIBRARY := -Wl,--whole-archive $(APP_DIR)/$(STATIC_TARGET) -Wl,--no-whole-archive $(UNICODE_LIBS) $(CUTIL_LIBS)


all: $(APP_DIR)/$(TARGET) $(APP_DIR)/$(STATIC_TARGET) $(APP_DIR)/tang$(EXE_EXTENSION) ## Build the shared and static libraries

####################################################################
# Dependency Inclusion
####################################################################
# Compiler-generated .d files (see -MMD -MP -MF in compile commands).
TEST_EXE_NAMES := testAllocator testUnicodeString testTangLanguageParse testTangLanguageExecuteSimple testTangLanguageExecuteComplex testTangLanguageLibrary testBinary test
TEST_DEPFILES := $(addprefix $(APP_DIR)/,$(TEST_EXE_NAMES:%=%.d))
TANG_CLI_DEP := $(APP_DIR)/tang.d
DEPFILES := $(LIBOBJECTS:.o=.d) $(TEST_DEPFILES) $(TANG_CLI_DEP)
# Sanitize existing .d files so Windows paths (C:/) are escaped before -include (fixes "multiple target patterns" on Linux when .d files came from a Windows build).
$(shell for f in $(DEPFILES); do test -f "$$f" && sed -i 's|\([A-Za-z]\):\([\\/]\)|\1\\:\2|g' "$$f"; done 2>/dev/null)
-include $(DEPFILES)

####################################################################
# Bison-Generated Files
####################################################################
$(GEN_DIR)/tangParser.h: \
				bison/tangParser.y \
				include/ghoti.io/tang/ast/astNode.h \
				include/ghoti.io/tang/location.h
	@echo "\n### Generating Bison TangParser ###"
	@mkdir -p $(@D)
	bison -v -o $(GEN_DIR)/tangParser.c -d $<

# Almost every translation unit reaches tangParser.h through
# include/ghoti.io/tang/ast/astNode.h, but nothing ordered them after bison. A serial
# build happened to generate the parser first; with -j the AST objects raced
# ahead and failed with "tangParser.h: No such file or directory". Order-only
# so a regenerated header does not force a full rebuild.
$(LIBOBJECTS): | $(GEN_DIR)/tangParser.h

# Ensure that tangParser.c is regenerated properly before
# tangParser.o tries to use it to compile.
$(GEN_DIR)/tangParser.c: $(GEN_DIR)/tangParser.h


####################################################################
# Flex-Generated Files
####################################################################
$(GEN_DIR)/htmlEscape.c: \
		flex/htmlEscape.l
	@echo "\n### Generating The HtmlEscape Scanner ###"
	@mkdir -p $(@D)
	flex -o $@ $<

$(GEN_DIR)/htmlEscapeAscii.c: \
		flex/htmlEscapeAscii.l \
		include/ghoti.io/tang/unicodeString.h \
		include/ghoti.io/tang/macros.h \
		include/ghoti.io/tang/libver.h
	@echo "\n### Generating The HtmlEscapeAscii Scanner ###"
	@mkdir -p $(@D)
	flex -o $@ $<

$(GEN_DIR)/percentEncode.c: \
		flex/percentEncode.l
	@echo "\n### Generating The PercentEncode Scanner ###"
	@mkdir -p $(@D)
	flex -o $@ $<

$(GEN_DIR)/tangScanner.c: \
		flex/tangScanner.l \
		$(GEN_DIR)/tangParser.h \
		include/ghoti.io/tang/unicodeString.h \
		include/ghoti.io/tang/macros.h \
		include/ghoti.io/tang/libver.h
	@echo "\n### Generating Flex TangScanner ###"
	@mkdir -p $(@D)
	flex -o $@ --header-file=$(GEN_DIR)/flexTangScanner.h $<

$(GEN_DIR)/unescape.c: \
		flex/unescape.l
	@echo "\n### Generating The Unescape Scanner ###"
	@mkdir -p $(@D)
	flex -o $@ $<

####################################################################
# Object Files
####################################################################
# Pattern rule: src/%.c -> objects/%.o; compiler generates .d (headers tracked automatically).
# Sed escapes Windows C: in .d so make does not treat "C" as a target.
####################################################################
# Generated version header
####################################################################

LIBVER_GEN := $(GEN_DIR)/ghoti.io/tang/libver_gen.h

# libver_gen.h is regenerated on every build and rewritten only when its content
# changes, so a variable given on the command line - make MAJOR_VERSION=2, or
# make BRANCH=-dev - takes effect. Keying the rule on the Makefile's timestamp
# alone left the previous token and version baked into the build, and nothing
# said so.
.PHONY: force-libver
force-libver:

$(LIBVER_GEN): force-libver
	@if [ -z "$(LIBVER_SYMBOL)" ]; then \
		printf "### LIBVER_SYMBOL is empty ###\n" >&2; exit 1; \
	fi
	@mkdir -p $(@D)
	@printf '%s\n' \
		'// Generated by the Makefile. Do not edit; see CONVENTIONS.md section 4.' \
		'#ifndef GHOTI_IO_GTA_LIBVER_GEN_H' \
		'#define GHOTI_IO_GTA_LIBVER_GEN_H' \
		'' \
		'/** The symbol namespace for this build, from the Makefile'"'"'s BRANCH. */' \
		'#define GHOTIIO_TANG_NAME $(LIBVER_SYMBOL)' \
		'' \
		'/** Human-readable version of this build. */' \
		'#define GHOTIIO_TANG_VERSION "$(VERSION_STRING)"' \
		'' \
		'/** The same version as three integers. */' \
		'#define GHOTIIO_TANG_VERSION_MAJOR $(MAJOR_VERSION)' \
		'#define GHOTIIO_TANG_VERSION_MINOR $(VERSION_MINOR_ONLY)' \
		'#define GHOTIIO_TANG_VERSION_PATCH $(VERSION_PATCH_ONLY)' \
		'' \
		'#endif // GHOTI_IO_GTA_LIBVER_GEN_H' > $@.tmp
	@if cmp -s $@.tmp $@; then rm -f $@.tmp; else mv $@.tmp $@; fi

$(OBJ_DIR)/%.o: src/%.c $(FLAGS_STAMP) | $(LIBVER_GEN)
	@printf "\n### Compiling $@ ###\n"
	@mkdir -p $(@D)
	$(CC) $(LIB_CFLAGS) $(INCLUDE) -c $< -MMD -MP -MF $(@:.o=.d) -o $@ $(OS_SPECIFIC_OBJECT_FLAGS)
	@sed -i 's|\([A-Za-z]\):\([\\/]\)|\1\\:\2|g' $(@:.o=.d) 2>/dev/null || true

# Generated sources (bison/flex): same flags, output .d next to .o.
#
# The order-only dependency on LIBVER_GEN is the same one the $(OBJ_DIR)/%.o
# pattern rule carries, and these two need it just as much: every translation
# unit includes macros.h, which reaches libver_gen.h. Without it a clean
# `make test` failed outright, because tangParser.o is the first entry in
# LIBOBJECTS and so was compiled before anything had generated that header.
#
# They carry the flags stamp for the same reason, and it has to be spelled out
# here because an explicit rule replaces the pattern rule's prerequisites
# rather than adding to them. Without it these two were the only objects in
# the tree that a flag change did not rebuild: 60 of 62 picked up
# `EXTRA_CFLAGS=-O1`, while tangParser.o and tangScanner.o kept recording -O2
# in their DW_AT_producer. They are also the two largest objects in the
# library, so the pair that silently stayed behind was the parser and the
# scanner. The stamp goes after the .c so that $< is still the source.
$(OBJ_DIR)/tangParser.o: $(GEN_DIR)/tangParser.c $(FLAGS_STAMP) | $(LIBVER_GEN)
	@printf "\n### Compiling $@ ###\n"
	@mkdir -p $(@D)
	$(CC) $(LIB_CFLAGS) $(INCLUDE) -c $< -MMD -MP -MF $(@:.o=.d) -o $@ $(OS_SPECIFIC_OBJECT_FLAGS) $(GENERATED_CFLAGS)
	@sed -i 's|\([A-Za-z]\):\([\\/]\)|\1\\:\2|g' $(@:.o=.d) 2>/dev/null || true

$(OBJ_DIR)/tangScanner.o: $(GEN_DIR)/tangScanner.c $(FLAGS_STAMP) | $(LIBVER_GEN)
	@printf "\n### Compiling $@ ###\n"
	@mkdir -p $(@D)
	$(CC) $(LIB_CFLAGS) $(INCLUDE) -c $< -MMD -MP -MF $(@:.o=.d) -o $@ $(OS_SPECIFIC_OBJECT_FLAGS) $(GENERATED_CFLAGS)
	@sed -i 's|\([A-Za-z]\):\([\\/]\)|\1\\:\2|g' $(@:.o=.d) 2>/dev/null || true

####################################################################
# Shared Library
####################################################################

$(APP_DIR)/$(STATIC_TARGET): $(LIBOBJECTS)
	@printf "\n### Archiving Tang Library ###\n"
	@mkdir -p $(@D)
	@rm -f $@
	ar rcs $@ $^

$(APP_DIR)/$(TARGET): \
		$(LIBOBJECTS)
	@printf "\n### Compiling Tang Shared Library ###\n"
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(LDFLAGS) $(OS_SPECIFIC_LIBRARY_NAME_FLAG)

ifeq ($(OS_NAME), Linux)
	@ln -f -s $(TARGET) $(APP_DIR)/$(SO_NAME)
	@ln -f -s $(SO_NAME) $(APP_DIR)/$(BASE_NAME)
endif

####################################################################
# Command Line Utility
####################################################################

$(APP_DIR)/tang$(EXE_EXTENSION): src/tang.c $(APP_DIR)/$(STATIC_TARGET) | $(APP_DIR)/$(TARGET)
	@printf "\n### Compiling Tang Command Line Utility ###\n"
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE) -MMD -MP -MF $(APP_DIR)/tang.d -o $@ $< $(LDFLAGS) $(TANGLIBRARY)

####################################################################
# Unit Tests
####################################################################

$(APP_DIR)/libtestLibrary.so: \
				test/libtestLibrary.cpp \
				$(APP_DIR)/$(TARGET) \
				include/ghoti.io/tang/tang.h
	@printf "\n### Compiling Test Library ###\n"
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -shared -o $@ $< $(LDFLAGS) -fPIC

# $< and the objects by name, not $^: -MMD writes the headers this test
# included into the .d, make reads them back as prerequisites of the same
# target, and $^ then puts every one of them on the command line. g++ compiles
# them and says nothing; clang++ refuses the whole link with "treating
# 'c-header' input as 'c++-header'". It only bites on the second build, once
# the .d exists.
$(APP_DIR)/testAllocator$(EXE_EXTENSION): test/test-allocator.cpp $(OBJ_DIR)/allocator.o
	@printf "\n### Compiling Allocator Test ###\n"
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -MMD -MP -MF $(APP_DIR)/testAllocator.d -o $@ $< $(OBJ_DIR)/allocator.o $(LDFLAGS) $(TESTFLAGS)

$(APP_DIR)/testUnicodeString$(EXE_EXTENSION): test/test-unicodeString.cpp $(OBJ_DIR)/unicodeString.o $(OBJ_DIR)/allocator.o
	@printf "\n### Compiling UnicodeString Test ###\n"
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -MMD -MP -MF $(APP_DIR)/testUnicodeString.d -o $@ $< $(OBJ_DIR)/unicodeString.o $(OBJ_DIR)/allocator.o $(LDFLAGS) $(TESTFLAGS)

$(APP_DIR)/testTangLanguageParse$(EXE_EXTENSION): test/test-tangLanguageParse.cpp $(APP_DIR)/$(STATIC_TARGET) | $(APP_DIR)/$(TARGET)
	@printf "\n### Compiling Tang Language Parse Test ###\n"
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -MMD -MP -MF $(APP_DIR)/testTangLanguageParse.d -o $@ $< $(LDFLAGS) $(TESTFLAGS) $(TANGLIBRARY)

$(APP_DIR)/testTangLanguageExecuteSimple$(EXE_EXTENSION): test/test-tangLanguageExecuteSimple.cpp $(APP_DIR)/$(STATIC_TARGET) | $(APP_DIR)/$(TARGET)
	@printf "\n### Compiling Tang Language Execution Simple Test ###\n"
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -MMD -MP -MF $(APP_DIR)/testTangLanguageExecuteSimple.d -o $@ $< $(LDFLAGS) $(TESTFLAGS) $(TANGLIBRARY)

# The JIT's "mmap refused me a page" arm is not reachable from any input, so
# the suite runs it by preloading a shim that refuses every executable
# mapping. Named beside the other test artifacts, never installed, and built
# only where LD_PRELOAD exists. $(FLAGS_STAMP) because this is compiled with
# $(CFLAGS) like everything else, and an explicit rule takes none of the
# pattern rule's prerequisites with it.
ifeq ($(UNAME_S), Linux)
MMAP_FAIL_SHIM := $(APP_DIR)/libmmapfail.so

$(MMAP_FAIL_SHIM): test/mmap-fail.c $(FLAGS_STAMP)
	@printf "\n### Compiling the mmap failure shim ###\n"
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $<
endif

$(APP_DIR)/testTangLanguageExecuteComplex$(EXE_EXTENSION): test/test-tangLanguageExecuteComplex.cpp $(APP_DIR)/$(STATIC_TARGET) | $(APP_DIR)/$(TARGET)
	@printf "\n### Compiling Tang Language Execution Complex Test ###\n"
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -MMD -MP -MF $(APP_DIR)/testTangLanguageExecuteComplex.d -o $@ $< $(LDFLAGS) $(TESTFLAGS) $(TANGLIBRARY)

$(APP_DIR)/testTangLanguageLibrary$(EXE_EXTENSION): test/test-tangLanguageLibrary.cpp $(APP_DIR)/$(STATIC_TARGET) | $(APP_DIR)/$(TARGET)
	@printf "\n### Compiling Tang Language Library Test ###\n"
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -MMD -MP -MF $(APP_DIR)/testTangLanguageLibrary.d -o $@ $< $(LDFLAGS) $(TESTFLAGS) $(TANGLIBRARY)

$(APP_DIR)/testBinary$(EXE_EXTENSION): test/test-binary.cpp $(APP_DIR)/$(STATIC_TARGET) | $(APP_DIR)/$(TARGET)
	@printf "\n### Compiling Binary JIT functions Test ###\n"
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -MMD -MP -MF $(APP_DIR)/testBinary.d -o $@ $< $(LDFLAGS) $(TESTFLAGS) $(TANGLIBRARY)

$(APP_DIR)/test$(EXE_EXTENSION): test/test.cpp $(APP_DIR)/$(STATIC_TARGET) | $(APP_DIR)/$(TARGET)
	@printf "\n### Compiling Tang Test ###\n"
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -MMD -MP -MF $(APP_DIR)/test.d -o $@ $< $(LDFLAGS) $(TESTFLAGS) $(TANGLIBRARY)

####################################################################
# Sanitizer build (ASan + UBSan)
####################################################################
#
# A second, separately-instrumented build of the whole library and every test.
# Six things here are deliberate, and each of them is a mistake some library
# in this suite has already made:
#
# 1. Its own build directory. Sanitizer objects and release objects must never
#    be linked together, and sharing $(OBJ_DIR) makes that a matter of which
#    target ran last.
#
# 2. Every compile writes a .d file and every one of them is included below.
#    Without that this build tracks source timestamps only: edit a header and
#    the objects including it are not rebuilt, so the run links objects
#    compiled against different versions of the same struct. compress hit
#    exactly this - one object sized a struct at 56 bytes and another at 64,
#    and ASan reported a heap-buffer-overflow in correct code. A sanitizer
#    build that can be assembled from mismatched objects is worse than none:
#    it can invent a failure and it can just as easily hide a real one.
#
# 3. float-cast-overflow is named explicitly. It is in *clang's*
#    -fsanitize=undefined group and NOT in GCC's, and -fno-sanitize-recover
#    names the same group, so it does not cover the check either. A gate that
#    says only "undefined" will print this class of bug and still exit 0.
#    Measured on gcc 14.2 by the model session; ctang has a live instance
#    (`x as int` for a large float), so this is not hypothetical here.
#
# 4. -fno-sanitize-recover at compile time, as well as halt_on_error=1 in the
#    environment. UBSan recovers by default: it prints the diagnostic, carries
#    on, and exits 0. The environment variable closes that too, but only for
#    runs that go through this target - the compile-time flag is what still
#    holds when someone runs one of these binaries by hand.
#
# 5. LD_PRELOAD names the ASan runtime, when the compiler has one to name.
#    GCC's runtime is a shared library that insists on being initialised
#    before anything it intercepts; any LD_PRELOAD inherited from the
#    environment loads ahead of it and it then refuses to start at all.
#    Desktop sessions here set LD_PRELOAD for unrelated reasons, so this is
#    not hypothetical either.
#
#    Clang links its runtime into the executable instead, and then refuses to
#    start when a second one arrives ahead of it. So the preload is decided
#    from the compiler rather than assumed: see SAN_PRELOAD below.
#
# 6. Run it under BOTH compilers - `make test-asan` and `make test-asan
#    CC=clang CXX=clang++` - because they do not implement the same checks and
#    the difference is not small. Until note 5 was written this target could
#    not start under clang at all, so every sanitizer figure this library had
#    was GCC's. Turning clang on found two things in the first run:
#
#      - 31 spellings of a member offset as `&((T *)0)->m`, which is a member
#        access on a null pointer. Clang's UBSan reports it; GCC's does not.
#        They are `offsetof` now.
#      - `-fsanitize=function`, which is in clang's -fsanitize=undefined group
#        and has no GCC equivalent for C, checks an indirect call by reading a
#        signature stored in front of the callee - and killed the process on
#        the first call into JIT-generated code, which has no such signature.
#        See gta_program_execute_binary.
#
#    Neither was reachable from the fuzz harnesses either, because those parse
#    and never execute, and the JIT compiler runs on no other instrumented
#    path. So "the sanitizers are clean" meant less than it read as.
#
# There is deliberately no TSan target: ctang creates no threads (pthread_create
# appears in no source file), so a ThreadSanitizer run could not fail, and a
# gate that cannot fail is worse than an absent one because it reads as
# coverage. Add one with the first thread.

SAN_CHECKS := undefined,float-cast-overflow
SAN_FLAGS := -fsanitize=address,$(SAN_CHECKS) \
             -fno-sanitize-recover=$(SAN_CHECKS) \
             -fno-omit-frame-pointer -g

SAN_BUILD_DIR := ./build/$(BUILD)-san
SAN_OBJ_DIR := $(SAN_BUILD_DIR)/objects
SAN_FLAGS_STAMP := $(SAN_OBJ_DIR)/.flags
SAN_APP_DIR := $(SAN_BUILD_DIR)/apps

SAN_CFLAGS := $(CFLAGS) $(SAN_FLAGS)
SAN_CXXFLAGS := $(CXXFLAGS) $(SAN_FLAGS)
SAN_LDFLAGS := $(LDFLAGS) $(SAN_FLAGS)
# The mirror of LIB_CFLAGS. It exists so that the stamp can record these two
# flags: spelled inline in the recipes, they were text no variable held, and a
# stamp cannot record what it cannot name.
SAN_LIB_CFLAGS := $(SAN_CFLAGS) -fvisibility=hidden -DGHOTIIO_TANG_BUILD

SAN_LIBOBJECTS := $(patsubst $(OBJ_DIR)/%,$(SAN_OBJ_DIR)/%,$(LIBOBJECTS))
SAN_STATIC_TARGET := $(SAN_APP_DIR)/$(STATIC_TARGET)
SAN_TANGLIBRARY := -Wl,--whole-archive $(SAN_STATIC_TARGET) -Wl,--no-whole-archive $(UNICODE_LIBS) $(CUTIL_LIBS)

# name|source, so one rule template covers them all.
SAN_TEST_PAIRS := \
	testAllocator|test/test-allocator.cpp \
	testUnicodeString|test/test-unicodeString.cpp \
	testTangLanguageParse|test/test-tangLanguageParse.cpp \
	testTangLanguageExecuteSimple|test/test-tangLanguageExecuteSimple.cpp \
	testTangLanguageExecuteComplex|test/test-tangLanguageExecuteComplex.cpp \
	testTangLanguageLibrary|test/test-tangLanguageLibrary.cpp \
	testBinary|test/test-binary.cpp

SAN_TEST_EXES := $(foreach p,$(SAN_TEST_PAIRS),$(SAN_APP_DIR)/$(word 1,$(subst |, ,$(p)))$(EXE_EXTENSION))

# See note 5. Which compiler this is cannot be read off -print-file-name:
# clang searches the GCC toolchain directory too, so it answers that query
# with GCC's libasan.so and the file is really there. Preloading it ahead of
# clang's own statically linked runtime is what produced
#
#     Your application is linked against incompatible ASan runtimes.
#
# from `make test-asan CC=clang`, before any test ran - which is why clang had
# never run this target, and why the sanitizer gate here had only ever been
# GCC's. -print-runtime-dir is the discriminator that holds: clang names the
# directory its runtimes live in, and GCC does not have the option at all and
# answers nothing.
#
# An empty LD_PRELOAD is still an assignment, so the clang branch is not a
# no-op: it clears whatever the desktop session put there, which is the half
# of note 5 that applies to both compilers.
SAN_COMPILER_RUNTIME_DIR := $(shell $(CC) -print-runtime-dir 2>/dev/null)
SAN_ASAN_RUNTIME := $(shell $(CC) -print-file-name=libasan.so)
SAN_PRELOAD := $(if $(SAN_COMPILER_RUNTIME_DIR),,$(SAN_ASAN_RUNTIME))
SAN_RUN_ENV := LD_LIBRARY_PATH="$(SAN_APP_DIR):$(LIB_INSTALL_PATH)/$(SUITE)" \
               LD_PRELOAD="$(SAN_PRELOAD)" \
               ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:abort_on_error=0 \
               UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1

$(SAN_OBJ_DIR)/%.o: src/%.c $(SAN_FLAGS_STAMP) | $(LIBVER_GEN)
	@mkdir -p $(@D)
	$(CC) $(SAN_LIB_CFLAGS) $(INCLUDE) -c $< -MMD -MP -MF $(@:.o=.d) -o $@

$(SAN_OBJ_DIR)/tangParser.o: $(GEN_DIR)/tangParser.c $(SAN_FLAGS_STAMP) | $(LIBVER_GEN)
	@mkdir -p $(@D)
	$(CC) $(SAN_LIB_CFLAGS) $(INCLUDE) -c $< -MMD -MP -MF $(@:.o=.d) -o $@ $(GENERATED_CFLAGS)

$(SAN_OBJ_DIR)/tangScanner.o: $(GEN_DIR)/tangScanner.c $(SAN_FLAGS_STAMP) | $(LIBVER_GEN)
	@mkdir -p $(@D)
	$(CC) $(SAN_LIB_CFLAGS) $(INCLUDE) -c $< -MMD -MP -MF $(@:.o=.d) -o $@ $(GENERATED_CFLAGS)

$(SAN_STATIC_TARGET): $(SAN_LIBOBJECTS)
	@printf "\n### Archiving instrumented library ###\n"
	@mkdir -p $(@D)
	@rm -f $@
	ar rcs $@ $^

# See note 2. Both the library objects and the test objects.
-include $(SAN_LIBOBJECTS:.o=.d)
-include $(SAN_TEST_EXES:$(EXE_EXTENSION)=.d)

define san-test-rule
$(SAN_APP_DIR)/$1$(EXE_EXTENSION): $2 $(SAN_STATIC_TARGET)
	@printf "\n### Compiling instrumented $1 ###\n"
	@mkdir -p $$(@D)
	$$(CXX) $$(SAN_CXXFLAGS) $$(INCLUDE) -MMD -MP -MF $(SAN_APP_DIR)/$1.d -o $$@ $2 $$(SAN_LDFLAGS) $$(TESTFLAGS) $$(SAN_TANGLIBRARY)
endef
$(foreach p,$(SAN_TEST_PAIRS),$(eval $(call san-test-rule,$(word 1,$(subst |, ,$(p))),$(word 2,$(subst |, ,$(p))))))

# The three language suites are run once per execution engine, exactly as the
# release `test` target does. Running each binary once would instrument only
# whichever engine is the default, and the two do not share a code path - the
# assignment defect fixed in e993ae2 was present in one and absent in the
# other. A single run also silently reports 125 assertions where `make test`
# reports 192, which is the kind of shortfall that reads as "it passed".
SAN_SINGLE_RUN := testAllocator testUnicodeString testTangLanguageParse
SAN_DUAL_RUN := testTangLanguageExecuteSimple testTangLanguageExecuteComplex testTangLanguageLibrary

test-asan: ## Run the tests under AddressSanitizer + UndefinedBehaviorSanitizer
test-asan: $(SAN_TEST_EXES)
ifeq ($(OS_NAME), Linux)
	@printf "\033[0;36m\n### Running tests under ASan + UBSan ###\033[0m\n\n"
	@for t in $(SAN_SINGLE_RUN); do \
		printf "\033[0;30;43m\n### %s ###\033[0m\n\n" "$$t"; \
		$(SAN_RUN_ENV) $(SAN_APP_DIR)/$$t$(EXE_EXTENSION) --gtest_brief=1 || exit 1; \
	done
	@printf "\033[0;30;43m\n### testBinary (JIT) ###\033[0m\n\n"
	@$(SAN_RUN_ENV) TANG_DISABLE_BINARY= $(SAN_APP_DIR)/testBinary$(EXE_EXTENSION) --gtest_brief=1 || exit 1
	@for t in $(SAN_DUAL_RUN); do \
		printf "\033[0;30;43m\n### %s (bytecode) ###\033[0m\n\n" "$$t"; \
		$(SAN_RUN_ENV) TANG_DISABLE_BINARY= $(SAN_APP_DIR)/$$t$(EXE_EXTENSION) --gtest_brief=1 || exit 1; \
		printf "\033[0;30;43m\n### %s (JIT) ###\033[0m\n\n" "$$t"; \
		$(SAN_RUN_ENV) TANG_DISABLE_BYTECODE= $(SAN_APP_DIR)/$$t$(EXE_EXTENSION) --gtest_brief=1 || exit 1; \
	done
	@printf "\033[0;32m\n### All tests passed under ASan + UBSan ###\033[0m\n"
else
	@printf "\033[0;31m\nSanitizer builds are only supported on Linux\n\033[0m\n"
	@exit 1
endif

# An instrumented build is worth nothing if it cannot be seen to fail, and the
# ways it silently cannot are the whole subject of the comment at the top of
# this section. This target plants one fault of each kind, in a scratch file
# outside the tree, and fails if the toolchain does not report it.
sanitizer-selftest: ## Prove the sanitizer flags actually catch what they claim
	@set -e; d=$$(mktemp -d); trap 'rm -rf $$d' EXIT; \
	printf '#include <stdlib.h>\nint main(void){char*p=malloc(4);p[5]=1;return p[5];}\n' > $$d/a.c; \
	printf '#include <limits.h>\nvolatile int a=INT_MAX;\nint main(void){return a+1;}\n' > $$d/b.c; \
	printf 'volatile double d=1e30;\nint main(void){return (int)d;}\n' > $$d/c.c; \
	fail=0; \
	for c in a:heap-overflow b:signed-overflow c:float-cast-overflow; do \
		src=$${c%%:*}; name=$${c##*:}; \
		$(CC) $(SAN_FLAGS) -o $$d/$$src $$d/$$src.c 2>/dev/null; \
		if env -u LD_PRELOAD ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
		   $$d/$$src >/dev/null 2>&1; then \
			printf "  \033[0;31mNOT CAUGHT\033[0m  %s (exited 0)\n" "$$name"; fail=1; \
		else \
			printf "  caught      %s\n" "$$name"; \
		fi; \
	done; \
	if [ $$fail -ne 0 ]; then \
		printf "\n\033[0;31mThe sanitizer flags do not catch what they claim.\033[0m\n" >&2; \
		exit 1; \
	fi; \
	printf "\nAll three fault kinds are reported and exit non-zero.\n"

####################################################################
# Fuzzing (libFuzzer, requires clang)
####################################################################
#
# The library objects are rebuilt with -fsanitize=fuzzer-no-link so libFuzzer
# can steer by the coverage it observes inside ctang. Against an uninstrumented
# library it would see only the harness and degrade into random byte
# generation, which for a grammar this size finds nothing.
#
# ASan and UBSan are on: a parser reading one byte past a buffer is exactly the
# bug being hunted and will not usually crash on its own. float-cast-overflow
# is named for the reason given in the sanitizer section, and
# -fno-sanitize-recover because a finding that only prints is an input
# libFuzzer never saves as an artifact - the campaign would run past the bug
# and report nothing.
#
# .d files here for the same reason as everywhere else: without them a header
# change rebuilds nothing, and objects that disagree about a struct's layout
# produce "findings" in correct code.
FUZZ_CC ?= clang
FUZZ_CXX ?= clang++
FUZZ_CC_OK := $(shell which $(FUZZ_CXX) 2>/dev/null)
# These flags instrument ctang's own objects and nothing else. cutil and unicode
# are linked as ordinary shared libraries, so ASan cannot see a bad access that
# happens inside them - it only notices once the damage reaches memory ctang
# owns, if it ever does. compress learned the same thing the same way.
#
# To see into cutil, build it with clang+ASan into a prefix of your own and
# link against that instead of the installed .so. Doing exactly that turned up
# two real null-pointer defects in cutil's hash template within seconds, both
# reached from ctang's simplify pass and neither visible to this target. The
# zero-setup version of the same question is to replay the corpus under
# valgrind, which instruments every library without rebuilding any of them.
FUZZ_SAN := -fsanitize=address,$(SAN_CHECKS) -fno-sanitize-recover=$(SAN_CHECKS) \
            -fno-omit-frame-pointer -g -O1
FUZZ_LIB_FLAGS := $(FUZZ_SAN) -fsanitize=fuzzer-no-link
FUZZ_BIN_FLAGS := $(FUZZ_SAN) -fsanitize=fuzzer
# As with SAN_LIB_CFLAGS: everything the fuzz recipes actually pass, in one
# variable, so that the stamp records the whole command and not a prefix of it.
# UNICODE_CFLAGS and CUTIL_CFLAGS matter most here - they come from pkg-config, so
# they change when a dependency is reinstalled rather than when anyone edits
# this file, and the fuzz tree is the one that runs longest against them.
FUZZ_LIB_CFLAGS := $(FUZZ_LIB_FLAGS) -std=c17 -w -DGHOTIIO_TANG_BUILD $(UNICODE_CFLAGS) $(CUTIL_CFLAGS)
FUZZ_BIN_CFLAGS := $(FUZZ_BIN_FLAGS) -std=c17 -w $(UNICODE_CFLAGS) $(CUTIL_CFLAGS)

FUZZ_DIR := $(BUILD_DIR)-fuzz
FUZZ_OBJ_DIR := $(FUZZ_DIR)/objects
FUZZ_FLAGS_STAMP := $(FUZZ_OBJ_DIR)/.flags
FUZZ_APP_DIR := $(FUZZ_DIR)/apps
# The corpus a run grows is working state: coverage-guided, hundreds of files,
# and regenerated from the seeds. Only the seeds are tracked, and they are
# copied in rather than fuzzed in place so a run never rewrites them.
FUZZ_SEEDS := test/fuzz/seeds
FUZZ_CORPUS := build/fuzz-corpus
# Crash and leak artifacts. libFuzzer writes these to the working directory
# unless told otherwise, which for a `make` run is the repository root - a
# reproducer for a real bug, dropped as an untracked file where the next
# `git add` sweeps it up or a `git clean` deletes it.
FUZZ_ARTIFACTS := test/fuzz/artifacts
# Long enough to be worth running, short enough to sit through. Override for a
# real campaign: make fuzz FUZZ_TIME=3600
FUZZ_TIME ?= 60
# Below what the machine can spare, not at libFuzzer's default. This is a
# shared workstation that runs several sanitizer campaigns at once and has no
# swap; a limit set above the free memory means the kernel reaches its limit
# before libFuzzer reaches this one, and a death by memory pressure is
# unattributable - no report, no artifact, nothing to reproduce. Raise it for a
# machine with room: make fuzz FUZZ_RSS_MB=4096
FUZZ_RSS_MB ?= 2048

# An allocation this library cannot have is a code path, not a stop.
#
# A template can ask for an array of any size it likes - `[1, 2] * 1000000000`
# is eight words of source - and the right answer is the out-of-memory value,
# which is what every allocating function here is written to return.  ASan's
# default is to treat a refused allocation as a fatal error instead, so the
# campaign ended on the first program that asked for too much and the code that
# answers the question was never reached.  With these two the allocator refuses
# and hands back NULL, which is the input that arm needs.
#
# The cap is well under FUZZ_RSS_MB so that the refusal happens before the
# process grows into libFuzzer's own limit, where the death is a killed process
# rather than a reported value.
# quarantine_size_mb because the differential harness ran out of memory with
# no leak to show for it. LSan is clean at exit and a single input replays in
# 35ms, so nothing was lost and no one program was large: what grew was ASan's
# own quarantine, which holds freed blocks back from reuse so that a
# use-after-free has something to land on. Two engines per input fill it twice
# as fast as the parse harnesses do. Measured over 20000 executions from a
# fixed corpus and seed:
#
#   default (256MB quarantine) ............... 1460Mb
#   quarantine_size_mb=64 ....................  751Mb
#   quarantine_size_mb=64:malloc_context_size=5  724Mb
#   malloc_context_size=5 .................... 1349Mb
#
# So the quarantine is the whole of it, and the shorter allocation traces are
# not worth 27Mb - every report would lose the stack that says where the block
# came from. The cost of the smaller quarantine is real and is the reason it
# is not smaller still: a use-after-free is caught only while the block is
# held back, and 64MB is a shorter window than the default. It suits this
# harness because each execution builds and destroys its own program and
# context, so a stale pointer has one execution to be caught in, not a
# campaign.
FUZZ_ASAN_OPTIONS ?= allocator_may_return_null=1:max_allocation_size_mb=512:quarantine_size_mb=64

FUZZ_OBJECTS := $(patsubst $(OBJ_DIR)/%,$(FUZZ_OBJ_DIR)/%,$(LIBOBJECTS))
-include $(FUZZ_OBJECTS:.o=.d)
# And the harnesses' own. They include headers of their own - lastInput.h, and
# the generator, which is most of the differential harness - and without this
# the link rule tracks only the .c file. Editing the generator would then
# leave the previous binary in place, and a fuzz run that "does not pick up a
# change" is indistinguishable from one that found the change made no
# difference.
FUZZ_HARNESSES := fuzz_parse fuzz_template fuzz_differential
-include $(patsubst %,$(FUZZ_APP_DIR)/%.d,$(FUZZ_HARNESSES))

ifdef PREFIX
FUZZ_RPATH := -Wl,-rpath,$(LIB_INSTALL_PATH)/$(SUITE)
endif

# -w because the harnesses are built by a different compiler than the library
# is warned for; ctang's -Werror set is tuned for gcc and clang disagrees about
# several of them in the generated parser.
$(FUZZ_OBJ_DIR)/%.o: src/%.c $(FUZZ_FLAGS_STAMP) | $(LIBVER_GEN)
	@mkdir -p $(@D)
	@$(FUZZ_CC) $(FUZZ_LIB_CFLAGS) $(INCLUDE) -c $< -MMD -MP -MF $(@:.o=.d) -o $@

$(FUZZ_OBJ_DIR)/tangParser.o: $(GEN_DIR)/tangParser.c $(FUZZ_FLAGS_STAMP) | $(LIBVER_GEN)
	@mkdir -p $(@D)
	@$(FUZZ_CC) $(FUZZ_LIB_CFLAGS) $(INCLUDE) -c $< -MMD -MP -MF $(@:.o=.d) -o $@

$(FUZZ_OBJ_DIR)/tangScanner.o: $(GEN_DIR)/tangScanner.c $(FUZZ_FLAGS_STAMP) | $(LIBVER_GEN)
	@mkdir -p $(@D)
	@$(FUZZ_CC) $(FUZZ_LIB_CFLAGS) $(INCLUDE) -c $< -MMD -MP -MF $(@:.o=.d) -o $@

# $1 = harness basename, $2 = target suffix
define fuzz-rule
fuzz-$2: ## Build the $2 fuzz harness (requires clang)
fuzz-$2: $$(FUZZ_APP_DIR)/$1

$$(FUZZ_APP_DIR)/$1: test/fuzz/$1.c $$(FUZZ_OBJECTS)
	@if [ -z "$$(FUZZ_CC_OK)" ]; then \
		echo "fuzzing requires $$(FUZZ_CXX); install clang or set FUZZ_CC/FUZZ_CXX" >&2; \
		exit 1; \
	fi
	@mkdir -p $$(@D) $$(FUZZ_CORPUS)/$2
	@printf "\n### Building $1 ###\n"
	$$(FUZZ_CC) $$(FUZZ_BIN_CFLAGS) $$(INCLUDE) \
		-MMD -MP -MF $$(FUZZ_APP_DIR)/$1.d \
		-o $$@ $$< $$(FUZZ_OBJECTS) $$(UNICODE_LIBS) $$(CUTIL_LIBS) -lstdc++ -lm $$(FUZZ_RPATH)

# env -u LD_PRELOAD for the same reason the sanitizer target does it: desktop
# sessions here set LD_PRELOAD for unrelated reasons and a sanitizer runtime
# insists on loading first. It is belt and braces under clang, which links the
# ASan runtime statically into the binary where nothing can displace it - a run
# with the desktop's LD_PRELOAD in place was measured clean over a million
# executions. That safety is an accident of the compiler, though: set FUZZ_CC
# to gcc and the runtime becomes a shared libasan.so, which does refuse to
# start behind another preloaded library.
fuzz-run-$2: ## Run the $2 fuzzer for $$(FUZZ_TIME) seconds
fuzz-run-$2: $$(FUZZ_APP_DIR)/$1
	@mkdir -p $$(FUZZ_CORPUS)/$2
	@cp -n $$(FUZZ_SEEDS)/$2/* $$(FUZZ_CORPUS)/$2/ 2>/dev/null || true
	@printf "\n### Fuzzing $2 for $$(FUZZ_TIME)s ###\n"
	@mkdir -p $$(FUZZ_ARTIFACTS)
	@env -u LD_PRELOAD ASAN_OPTIONS=$$(FUZZ_ASAN_OPTIONS) $$(FUZZ_APP_DIR)/$1 $$(FUZZ_CORPUS)/$2 \
		-max_total_time=$$(FUZZ_TIME) \
		-timeout=10 -rss_limit_mb=$$(FUZZ_RSS_MB) -print_final_stats=1 \
		-artifact_prefix=$$(FUZZ_ARTIFACTS)/$2-
endef

$(eval $(call fuzz-rule,fuzz_parse,parse))
$(eval $(call fuzz-rule,fuzz_template,template))
$(eval $(call fuzz-rule,fuzz_differential,differential))

fuzz: ## Build and run every fuzzer for $(FUZZ_TIME) seconds each
fuzz: fuzz-run-parse fuzz-run-template fuzz-run-differential

fuzz-clean: ## Remove the fuzz build (keeps the corpus)
fuzz-clean:
	-@rm -rf $(FUZZ_DIR)

####################################################################
# Commands
####################################################################

# General commands
.PHONY: clean cloc docs docs-pdf coverage
# Release build commands
.PHONY: all install test test-watch uninstall watch jit-alignment-check check-symbols
# Sanitizer build commands
.PHONY: test-asan sanitizer-selftest
# Fuzzing commands
.PHONY: fuzz fuzz-clean
# Debug build commands
.PHONY: all-debug install-debug test-debug test-watch-debug uninstall-debug watch-debug


watch: ## Watch the file directory for changes and compile the target
	@while true; do \
          make --no-print-directory $(GEN_DIR)/tangParser.h; \
					make --no-print-directory all; \
					printf "\033[0;32m\n"; \
					printf "#########################\n"; \
					printf "# Waiting for changes.. #\n"; \
					printf "#########################\n"; \
					printf "\033[0m\n"; \
					inotifywait -qr -e modify -e create -e delete -e move src include bison flex test Makefile --exclude '/\.'; \
					done

test-watch: ## Watch the file directory for changes and run the unit tests
	@while true; do \
		make --no-print-directory all; \
		make --no-print-directory test; \
		printf "\033[0;32m\n"; \
		printf "#########################\n"; \
		printf "# Waiting for changes.. #\n"; \
		printf "#########################\n"; \
		printf "\033[0m\n"; \
		inotifywait -qr -e modify -e create -e delete -e move src include bison flex test Makefile --exclude '/\.'; \
		done

# So tests can load the tang library and its cutil dependency. cutil is found
# where it was installed, which is the only way one library here finds
# another; the second entry is the same one every sibling Makefile uses.
TEST_LD_PATH := $(APP_DIR):$(LIB_INSTALL_PATH)/$(SUITE)

jit-alignment-check: ## Rebuild with the JIT stack-alignment check and run the tests
# The System V AMD64 ABI requires rsp to be 16-byte aligned at every call. The
# JIT prologue establishes that, so alignment holds in the body only while an
# even number of 8-byte slots are live. Getting it wrong is silent until some
# callee happens to use an alignment-sensitive instruction (movdqa, say) on a
# stack local, at which point it faults deep inside that callee with nothing
# pointing back at the offending call.
#
# This target rebuilds with -DGTA_JIT_CHECK_STACK_ALIGNMENT, which makes every
# emitted call test rsp first and execute ud2 (SIGILL) when it is misaligned.
# A violation then traps AT the offending call. Run this after changing any
# code that emits pushes or pops around a call.
	$(MAKE) clean
	$(MAKE) test \
		CFLAGS="$(CFLAGS) -DGTA_JIT_CHECK_STACK_ALIGNMENT" \
		CXXFLAGS="$(CXXFLAGS) -DGTA_JIT_CHECK_STACK_ALIGNMENT"

####################################################################
# Symbol namespace check
####################################################################

check-symbols: ## Fail if any exported symbol lacks the version namespace
check-symbols: $(APP_DIR)/$(TARGET)
ifeq ($(OS_NAME), Linux)
	@leaked=$$(nm -D --defined-only $(APP_DIR)/$(TARGET) \
		| awk '$$2 ~ /^[TDBR]$$/ {print $$3}' \
		| grep -v '^$(LIBVER_SYMBOL)_' | grep -v '^_' || true); \
	if [ -n "$$leaked" ]; then \
		printf "\033[0;31m\n### Exported symbols missing the $(LIBVER_SYMBOL)_ namespace ###\033[0m\n" >&2; \
		printf "%s\n" "$$leaked" >&2; \
		printf "\nEach needs a '#define <name> GHOTIIO_TANG(<name>)' line in namespace.h, or\n" >&2; \
		printf "should not be exported at all. See CONVENTIONS.md section 4.\n" >&2; \
		exit 1; \
	fi
	@split=$$(nm -D --undefined-only $(APP_DIR)/$(TARGET) \
		| awk '{print $$2}' | grep '^$(LIBVER_SYMBOL)_' || true); \
	if [ -n "$$split" ]; then \
		printf "\033[0;31m\n### Renamed but undefined - a split symbol ###\033[0m\n" >&2; \
		printf "%s\n" "$$split" >&2; \
		printf "\nA translation unit referenced the namespaced name while the one that\n" >&2; \
		printf "defines it did not see the rename.\n" >&2; \
		exit 1; \
	fi
	@unexported=$$(find include -name '*.h' -exec awk '/^#if DOXYGEN/{d=1} d==0 && /^[a-z_][A-Za-z0-9_ ]*\**[[:space:]]*gta_[a-z0-9_]+[[:space:]]*\(/{print FILENAME": "$$0} /^#endif/{d=0}' {} + \
		| grep -vE 'typedef|static inline' || true); \
	if [ -n "$$unexported" ]; then \
		printf "\033[0;31m\n### Public declarations without GTA_API ###\033[0m\n" >&2; \
		printf "%s\n" "$$unexported" >&2; \
		printf "\nThe library builds with -fvisibility=hidden, so these are not exported\n" >&2; \
		printf "and a consumer linking the .so gets an undefined reference. The tests\n" >&2; \
		printf "link the archive and would not notice.\n" >&2; \
		exit 1; \
	fi
	@nomacros=$$(find include src -name '*.h' \
		! -name 'libver.h' ! -name 'libver_gen.h' ! -name 'namespace.h' ! -name 'macros.h' \
		-exec grep -L '#include <ghoti.io/tang/macros.h>' {} + || true); \
	if [ -n "$$nomacros" ]; then \
		printf "\033[0;31m\n### Headers that do not include macros.h ###\033[0m\n" >&2; \
		printf "%s\n" "$$nomacros" >&2; \
		printf "\nEvery header must include <ghoti.io/tang/macros.h> before it declares\n" >&2; \
		printf "anything, so that the renames in namespace.h are already in effect. A\n" >&2; \
		printf "header that skips it can name a type before that type has been renamed,\n" >&2; \
		printf "producing two different types under one spelling.\n" >&2; \
		printf "See CONVENTIONS.md section 4.\n" >&2; \
		exit 1; \
	fi
	@badguards=$$(find include src -name '*.h' -exec awk 'FNR==1{d=0} !d && /^#ifndef/{print $$2; d=1}' {} + \
		| awk '$$1 !~ /^GHOTI_IO_GTA_/ {print $$1}' || true); \
	if [ -n "$$badguards" ]; then \
		printf "\033[0;31m\n### Include guards with the wrong prefix ###\033[0m\n" >&2; \
		printf "%s\n" "$$badguards" >&2; \
		printf "\nGuards mirror the path: GHOTI_IO_GTA_<PATH>_H. A guard without the\n" >&2; \
		printf "library token is one rename away from colliding with another library's.\n" >&2; \
		exit 1; \
	fi
	@dupguards=$$(find include src -name '*.h' -exec awk 'FNR==1{d=0} !d && /^#ifndef/{print $$2; d=1}' {} + \
		| sort | uniq -d || true); \
	if [ -n "$$dupguards" ]; then \
		printf "\033[0;31m\n### Headers sharing an include guard ###\033[0m\n" >&2; \
		printf "%s\n" "$$dupguards" >&2; \
		printf "\nTwo headers with one guard means whichever is included second is\n" >&2; \
		printf "silently empty. Guards mirror the path: GHOTI_IO_GTA_<PATH>_H.\n" >&2; \
		exit 1; \
	fi
	@printf "\033[0;32mEvery exported symbol carries the $(LIBVER_SYMBOL)_ namespace.\033[0m\n"
	@printf "\033[0;32mEvery public declaration carries GTA_API.\033[0m\n"
	@printf "\033[0;32mEvery header includes macros.h.\033[0m\n"
	@printf "\033[0;32mEvery include guard is unique and correctly prefixed.\033[0m\n"
else
	@printf "check-symbols: skipped (Linux only)\n"
endif

test: ## Make and run the Unit tests
test: \
				$(APP_DIR)/$(TARGET) \
				$(APP_DIR)/testAllocator$(EXE_EXTENSION) \
				$(APP_DIR)/testUnicodeString$(EXE_EXTENSION) \
				$(APP_DIR)/testTangLanguageParse$(EXE_EXTENSION) \
				$(APP_DIR)/testTangLanguageExecuteSimple$(EXE_EXTENSION) \
				$(APP_DIR)/testTangLanguageExecuteComplex$(EXE_EXTENSION) \
				$(APP_DIR)/testTangLanguageLibrary$(EXE_EXTENSION) \
				$(APP_DIR)/testBinary$(EXE_EXTENSION) \
				$(APP_DIR)/tang$(EXE_EXTENSION) \
				$(MMAP_FAIL_SHIM) \
				$(TEST_GATES)
#				$(APP_DIR)/libtestLibrary.so \
#				$(APP_DIR)/test$(EXE_EXTENSION) \

	@printf "\033[0;30;103m\n"
	@printf "###############################\n"
	@printf "### Running allocator tests ###\n"
	@printf "###############################\n"
	@printf "\033[0m\n\n"
	LD_LIBRARY_PATH="$(TEST_LD_PATH)" $(APP_DIR)/testAllocator --gtest_brief=1
	@printf "\033[0;30;103m\n"
	@printf "############################\n"
	@printf "### Running string tests ###\n"
	@printf "############################\n"
	@printf "\033[0m\n\n"
	LD_LIBRARY_PATH="$(TEST_LD_PATH)" $(APP_DIR)/testUnicodeString --gtest_brief=1
	@printf "\033[0;30;43m\n"
	@printf "####################################\n"
	@printf "### Running Language Parse tests ###\n"
	@printf "####################################\n"
	@printf "\033[0m\n\n"
	LD_LIBRARY_PATH="$(TEST_LD_PATH)" $(APP_DIR)/testTangLanguageParse --gtest_brief=1
	@printf "\033[0;30;43m\n"
	@printf "################################\n"
	@printf "### Running Binary JIT tests ###\n"
	@printf "################################\n"
	@printf "\033[0m\n\n"
	LD_LIBRARY_PATH="$(TEST_LD_PATH)" TANG_DISABLE_BINARY= $(APP_DIR)/testBinary --gtest_brief=1

	@printf "\033[0;30;104m\n"
	@printf "########################################################\n"
	@printf "### Running Bytecode Language Execution Simple tests ###\n"
	@printf "########################################################\n"
	@printf "\033[0m\n\n"
	LD_LIBRARY_PATH="$(TEST_LD_PATH)" TANG_DISABLE_BINARY= $(APP_DIR)/testTangLanguageExecuteSimple --gtest_brief=1
	@printf "\033[0;30;104m\n"
	@printf "#########################################################\n"
	@printf "### Running Bytecode Language Execution Complex tests ###\n"
	@printf "#########################################################\n"
	@printf "\033[0m\n\n"
	LD_LIBRARY_PATH="$(TEST_LD_PATH)" TANG_DISABLE_BINARY= $(APP_DIR)/testTangLanguageExecuteComplex --gtest_brief=1
	@printf "\033[0;30;104m\n"
	@printf "################################################\n"
	@printf "### Running Bytecode Language Library tests  ###\n"
	@printf "################################################\n"
	@printf "\033[0m\n\n"
	LD_LIBRARY_PATH="$(TEST_LD_PATH)" TANG_DISABLE_BINARY= $(APP_DIR)/testTangLanguageLibrary --gtest_brief=1

	@printf "\033[0;30;45m\n"
	@printf "########################################################\n"
	@printf "### Running Binary Language Execution Simple tests   ###\n"
	@printf "########################################################\n"
	@printf "\033[0m\n\n"
	LD_LIBRARY_PATH="$(TEST_LD_PATH)" TANG_DISABLE_BYTECODE= $(APP_DIR)/testTangLanguageExecuteSimple --gtest_brief=1
	@printf "\033[0;30;45m\n"
	@printf "########################################################\n"
	@printf "### Running Binary Language Execution Complex tests  ###\n"
	@printf "########################################################\n"
	@printf "\033[0m\n\n"
	LD_LIBRARY_PATH="$(TEST_LD_PATH)" TANG_DISABLE_BYTECODE= $(APP_DIR)/testTangLanguageExecuteComplex --gtest_brief=1
	@printf "\033[0;30;45m\n"
	@printf "##############################################\n"
	@printf "### Running Binary Language Library tests  ###\n"
	@printf "##############################################\n"
	@printf "\033[0m\n\n"
	LD_LIBRARY_PATH="$(TEST_LD_PATH)" TANG_DISABLE_BYTECODE= $(APP_DIR)/testTangLanguageLibrary --gtest_brief=1

	@printf "\033[0;30;45m\n"
	@printf "##############################################\n"
	@printf "### Running with every JIT mapping refused ###\n"
	@printf "##############################################\n"
	@printf "\033[0m\n\n"
ifeq ($(UNAME_S), Linux)
# Every program falls back to the bytecode, so the whole suite must pass with
# no JIT at all. Both engines stay enabled: this asks what happens when the
# JIT is refused, which is not what TANG_DISABLE_BINARY asks.
	LD_LIBRARY_PATH="$(TEST_LD_PATH)" LD_PRELOAD="$(MMAP_FAIL_SHIM)" $(APP_DIR)/testTangLanguageExecuteSimple --gtest_brief=1
	LD_LIBRARY_PATH="$(TEST_LD_PATH)" LD_PRELOAD="$(MMAP_FAIL_SHIM)" $(APP_DIR)/testTangLanguageExecuteComplex --gtest_brief=1
else
	@printf "skipped (needs LD_PRELOAD)\n"
endif

	@printf "\033[0;30;47m\n"
	@printf "#########################\n"
	@printf "### Running CLI tests ###\n"
	@printf "#########################\n"
	@printf "\033[0m\n\n"
	LD_LIBRARY_PATH="$(TEST_LD_PATH)" $(SHELL) ./test/cli-test.sh $(APP_DIR)/tang$(EXE_EXTENSION)
#	@printf "\033[0;32m\n"
#	@printf "############################\n"
#	@printf "### Running normal tests ###\n"
#	@printf "############################\n"
#	@printf "\033[0m\n"
#	LD_LIBRARY_PATH="$(TEST_LD_PATH)" $(APP_DIR)/test --gtest_brief=1

clean: ## Remove all contents of the build directory for this OS/build.
# Both trees. The sanitizer build lives beside the release one rather than
# inside it, so a clean that names only $(BUILD_DIR) leaves it behind - which
# is how instrumented objects compiled against a previous version of a
# dependency's headers survive the command whose whole purpose is to remove
# them, and get linked into the next run. Every directory this Makefile
# creates under build/ belongs on this line.
	-@rm -rvf $(BUILD_DIR) $(SAN_BUILD_DIR) $(FUZZ_DIR)

# Files will be as follows:
# /usr/local/lib/(SUITE)/
#   lib(SUITE)-(PROJECT)(BRANCH).so.(MAJOR).(MINOR)
#   lib(SUITE)-(PROJECT)(BRANCH).so.(MAJOR) link to previous
#   lib(SUITE)-(PROJECT)(BRANCH).so link to previous
# Where the dynamic loader configuration fragment goes. Overridable so a
# staged or user-prefix install has somewhere to write it; the default is the
# system location, which is what an ordinary `sudo make install` uses.
LDCONF_INSTALL_PATH ?= /etc/ld.so.conf.d

# What goes in the .pc Requires: field. Built from the same variables the
# compile uses, so a dependency on another branch cannot be named one way for
# the build and another way for consumers.
PC_REQUIRES := $(CUTIL_PC) $(UNICODE_PC)

# Where this project's own .pc file is installed. Defaults to the directory
# pkg-config is already being told to search, but separate from it so a
# staged install can write somewhere else without also redirecting lookups.
PKGCONFIG_INSTALL_PATH ?= $(PC_INSTALL_PATH)

# $(LDCONF_INSTALL_PATH)/(SUITE)-(PROJECT)(BRANCH).conf will point to $(LIB_INSTALL_PATH)/(SUITE)
# /usr/local/include/(SUITE)/(PROJECT)(BRANCH)
#   *.h copied from ./include/(PROJECT)
# /usr/local/share/pkgconfig
#   (SUITE)-(PROJECT)(BRANCH).pc created

install: ## Install the library globally, requires sudo
# Depends on all: install used to copy whatever happened to be in the build
# directory, so it could install a stale artifact or fail outright on a clean
# tree.
install: all
	# Installing the shared library.
	@mkdir -p $(LIB_INSTALL_PATH)/$(SUITE)
ifeq ($(OS_NAME), Linux)
# Install the .so file
	@cp $(APP_DIR)/$(TARGET) $(LIB_INSTALL_PATH)/$(SUITE)/
	@ln -f -s $(TARGET) $(LIB_INSTALL_PATH)/$(SUITE)/$(SO_NAME)
	@ln -f -s $(SO_NAME) $(LIB_INSTALL_PATH)/$(SUITE)/$(BASE_NAME)
	# Installing the ld configuration file.
	@if [ -n "$(LDCONF_INSTALL_PATH)" ]; then mkdir -p $(LDCONF_INSTALL_PATH); fi
	@if [ -n "$(LDCONF_INSTALL_PATH)" ]; then echo "$(LIB_INSTALL_PATH)/$(SUITE)" > $(LDCONF_INSTALL_PATH)/$(SUITE)-$(PROJECT)$(BRANCH).conf; fi
endif
ifeq ($(OS_NAME), Windows)
# The .dll goes in bin/, where the loader finds it once that directory is on
# PATH - Windows has no rpath. The import library goes where the .pc's -L
# points, lib/$(SUITE)/, as the .so does on Linux; in lib/ no -L named it.
	@mkdir -p $(BIN_INSTALL_PATH) $(LIB_INSTALL_PATH)/$(SUITE)
	@cp $(APP_DIR)/$(TARGET).a $(LIB_INSTALL_PATH)/$(SUITE)/
	@cp $(APP_DIR)/$(TARGET) $(BIN_INSTALL_PATH)/
endif
	# Installing the headers.
	# Removed first: this directory is owned entirely by this project and
	# branch, and copying over the top of it would leave headers behind that
	# have since been renamed or deleted.
	@rm -rf $(INCLUDE_INSTALL_PATH)/$(SUITE)/$(PROJECT)$(BRANCH)
	@mkdir -p $(INCLUDE_INSTALL_PATH)/$(SUITE)/$(PROJECT)$(BRANCH)
	# Copied with --parents so the subdirectories survive. This used to be a
	# flat `cp include/ghoti.io/tang/*.h`, which installed 10 of the 68 headers
	# and none of the generated ones: tang.h includes program/program.h, so the
	# installed library could not actually be included by a consumer.
	@cd include && find . -name "*.h" -exec cp --parents '{}' $(INCLUDE_INSTALL_PATH)/$(SUITE)/$(PROJECT)$(BRANCH)/ \;
	@cd $(GEN_DIR) && find . -name "*.h" -exec cp --parents '{}' $(INCLUDE_INSTALL_PATH)/$(SUITE)/$(PROJECT)$(BRANCH)/ \;
	# Installing the pkg-config files.
	@mkdir -p $(PKGCONFIG_INSTALL_PATH)
	@cat pkgconfig/$(SUITE)-$(PROJECT).pc | sed 's/(SUITE)/$(SUITE)/g; s/(PROJECT)/$(PROJECT)/g; s/(BRANCH)/$(BRANCH)/g; s/(VERSION)/$(VERSION)/g; s|(LIB)|$(LIB_INSTALL_PATH)|g; s|(INCLUDE)|$(INCLUDE_INSTALL_PATH)|g; s|(REQUIRES)|$(PC_REQUIRES)|g' > $(PKGCONFIG_INSTALL_PATH)/$(SUITE)-$(PROJECT)$(BRANCH).pc
ifeq ($(OS_NAME), Linux)
	# Running ldconfig.
	@if [ -n "$(LDCONF_INSTALL_PATH)" ]; then ldconfig >> /dev/null 2>&1; fi
endif
	@echo "Ghoti.io $(PROJECT)$(BRANCH) installed"

uninstall: ## Delete the globally-installed files.  Requires sudo.
	# Deleting the shared library.
ifeq ($(OS_NAME), Linux)
	@rm -f $(LIB_INSTALL_PATH)/$(SUITE)/$(BASE_NAME)*
	# Deleting the ld configuration file.
	@rm -f $(LDCONF_INSTALL_PATH)/$(SUITE)-$(PROJECT)$(BRANCH).conf
endif
ifeq ($(OS_NAME), Windows)
	@rm -f $(LIB_INSTALL_PATH)/$(SUITE)/$(TARGET).a
	@rm -f $(BIN_INSTALL_PATH)/$(TARGET)
endif
	# Deleting the headers.
	@rm -rf $(INCLUDE_INSTALL_PATH)/$(SUITE)/$(PROJECT)$(BRANCH)
	# Deleting the pkg-config files.
	@rm -f $(PKGCONFIG_INSTALL_PATH)/$(SUITE)-$(PROJECT)$(BRANCH).pc
	# Cleaning up (potentially) no longer needed directories.
	@rmdir --ignore-fail-on-non-empty $(INCLUDE_INSTALL_PATH)/$(SUITE)
	@rmdir --ignore-fail-on-non-empty $(LIB_INSTALL_PATH)/$(SUITE)
ifeq ($(OS_NAME), Linux)
	# Running ldconfig.
	@if [ -n "$(LDCONF_INSTALL_PATH)" ]; then ldconfig >> /dev/null 2>&1; fi
endif
	@echo "Ghoti.io $(PROJECT)$(BRANCH) has been uninstalled"

debug: ## Build the project in DEBUG mode
	make all BUILD=debug

install-debug: ## Install the DEBUG library globally, requires sudo
	make install BUILD=debug

uninstall-debug: ## Delete the DEBUG globally-installed files.  Requires sudo.
	make uninstall BUILD=debug

test-debug: ## Make and run the Unit tests in DEBUG mode
	make test BUILD=debug

watch-debug: ## Watch the file directory for changes and compile the target in DEBUG mode
	make watch BUILD=debug

test-watch-debug: ## Watch the file directory for changes and run the unit tests in DEBUG mode
	make test-watch BUILD=debug

docs: ## Generate the documentation in the ./docs subdirectory
	doxygen

docs-pdf: docs ## Generate the documentation as a pdf, at ./docs/(SUITE)-(PROJECT)(BRANCH).pdf
	cd ./docs/latex/ && make
	mv -f ./docs/latex/refman.pdf ./docs/$(SUITE)-$(PROJECT)$(BRANCH)-docs.pdf

cloc: ## Count the lines of code used in the project
	cloc src include flex bison test Makefile

coverage: ## Build instrumented, run the tests, and report line coverage
# Cleans first because the object files would otherwise be reused without the
# instrumentation, then cleans and rebuilds at the end: leaving the
# instrumented objects behind would have a later `make` silently link them,
# and leaving the tree cleaned would break any sibling project that links
# this one. The cost is one extra build; coverage is not run often.
	@$(MAKE) --no-print-directory clean > /dev/null
# The instrumented build, the report and the restoration of the tree are one
# shell command so that the cleanup runs whatever fails. Letting a failure
# stop the recipe leaves the --coverage objects in build/, and the next
# ordinary `make` links them into a library that needs the gcov runtime; every
# later build then fails with undefined references to __gcov_init until
# somebody works out why.
#
# TEST_GATES is cleared because --coverage links the gcov runtime, which
# exports mangle_path. check-symbols is right to reject that in a shipping
# build and wrong to reject it here, and it made this target fail before it
# ever produced a report.
	@status=0; \
	$(MAKE) --no-print-directory test TEST_GATES= \
		EXTRA_CFLAGS="--coverage -O0" \
		EXTRA_LDFLAGS="--coverage" > /dev/null || status=$$?; \
	if [ $$status -eq 0 ]; then \
		tools/coverage.sh $(OBJ_DIR) || status=$$?; \
	else \
		printf "coverage: the instrumented test run failed; no report\n" >&2; \
	fi; \
	$(MAKE) --no-print-directory clean > /dev/null; \
	$(MAKE) --no-print-directory all > /dev/null; \
	exit $$status

help: ## Display this help
# Scan only this makefile. $(MAKEFILE_LIST) grows to include every generated
# .d file once the project has been built, and grep prefixes each match with
# a filename when given more than one file - so every target name in the
# output became "Makefile".
	@grep -E '^[ a-zA-Z_-]+:.*?## .*$$' $(firstword $(MAKEFILE_LIST)) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "%-15s %s\n", $$1, $$2}' | sed "s/(SUITE)/$(SUITE)/g; s/(PROJECT)/$(PROJECT)/g; s/(BRANCH)/$(BRANCH)/g"


####################################################################
# Flag stamps
####################################################################
# Each build tree carries the flag string it was built with. The stamp is
# rewritten only when that string differs -- written to a scratch file,
# compared, moved into place only on a difference -- so its mtime moves on a
# flag change and on nothing else. The object rules above depend on it.
#
# This replaces listing `Makefile` as a prerequisite, which was too broad (a
# comment-only edit recompiled everything) and too narrow (a command-line
# override such as `make EXTRA_CFLAGS=-O2` changes no file's mtime and so was
# invisible).
#
# These rules sit at the end of the file for two reasons. A rule's target
# expands when make reads the line, so a stamp rule above its own OBJ_DIR
# definition has an empty target: not an error, just a rule that silently does
# not exist. And the first target in a makefile is the default goal, so a stamp
# rule above `all:` makes a bare `make` build the stamp and nothing else.
#
# Each stamp must name every variable its guarded recipes expand, the compiler
# included - a stamp that records a variable one derivation upstream of the
# real one passes every test you would think to run. Measured here before the
# fix: `make all CC='cc -O1'` rebuilt 0 of 62 objects and left every one of
# them recording -O2, because $(CC) appeared in every compile recipe and in no
# stamp. The same held for LIB_CFLAGS against CFLAGS, and for the fuzz tree's
# UNICODE_CFLAGS and CUTIL_CFLAGS, which come from pkg-config and so change when a
# dependency is reinstalled rather than when this file is edited.
#
# The check is mechanical: for each recipe a stamp guards, subtract the
# stamp's variables from the recipe's. Anything left is a flag change that
# rebuilds nothing, which is invisible - it looks exactly like a tree that was
# already current.
.PHONY: force-flags

$(FLAGS_STAMP): force-flags
	@mkdir -p $(@D)
	@printf '%s\n' '$(CC) $(CXX) $(LIB_CFLAGS) $(CXXFLAGS) $(LDFLAGS) $(TESTFLAGS) $(INCLUDE) $(OS_SPECIFIC_OBJECT_FLAGS) $(GENERATED_CFLAGS)' > $@.new
	@cmp -s $@.new $@ 2>/dev/null && rm -f $@.new || mv -f $@.new $@

$(FUZZ_FLAGS_STAMP): force-flags
	@mkdir -p $(@D)
	@printf '%s\n' '$(FUZZ_CC) $(FUZZ_LIB_CFLAGS) $(FUZZ_BIN_CFLAGS) $(INCLUDE)' > $@.new
	@cmp -s $@.new $@ 2>/dev/null && rm -f $@.new || mv -f $@.new $@

$(SAN_FLAGS_STAMP): force-flags
	@mkdir -p $(@D)
	@printf '%s\n' '$(CC) $(CXX) $(SAN_LIB_CFLAGS) $(SAN_CXXFLAGS) $(SAN_LDFLAGS) $(TESTFLAGS) $(INCLUDE) $(GENERATED_CFLAGS)' > $@.new
	@cmp -s $@.new $@ 2>/dev/null && rm -f $@.new || mv -f $@.new $@
