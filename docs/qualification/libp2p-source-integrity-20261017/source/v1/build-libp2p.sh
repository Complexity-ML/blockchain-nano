#!/bin/sh
set -eu
source_only=${NANO_LIBP2P_SOURCE_ONLY-0}
case "$source_only" in
    0|1) ;;
    *) echo "NANO_LIBP2P_SOURCE_ONLY must be 0 or 1" >&2; exit 2 ;;
esac
# Required by the byte-level source guard; Git alone does not guarantee Perl.
if ! command -v perl >/dev/null 2>&1 ||
   ! perl -MDigest::SHA -MCwd -MFcntl -e 'exit 0'; then
    echo "build-libp2p requires Perl with Digest::SHA, Cwd and Fcntl" >&2
    exit 2
fi
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
deps=${NANO_DEPS_DIR:-"$root/.deps"}
cmake_bin=${CMAKE_BIN:-cmake}
source_dir="$deps/cpp-libp2p"
commit=c03bf7c44c4fd30a390f3ef93bd1eab482e686a6
mkdir -p "$deps"
if [ ! -d "$source_dir/.git" ]; then
    git clone --depth 1 --branch v0.1.37 https://github.com/libp2p/cpp-libp2p.git "$source_dir"
fi
# Read the pinned tree and real files, not the worktree index/status cache.
# Only root .git metadata is reserved inside source; outputs/caches are siblings.
source_guard() {
    perl - "$source_dir" "$commit" <<'PERL'
use strict;
use warnings;
use Cwd qw(abs_path);
use Digest::SHA qw(sha1_hex);
use Fcntl qw(O_RDONLY O_NOFOLLOW O_NONBLOCK S_ISREG S_ISDIR S_ISLNK);
my ($literal, $commit) = @ARGV;
sub need { die "cpp-libp2p source guard: $_[1]\n" unless $_[0]; }
need(defined($commit) && $commit =~ /\A[0-9a-f]{40}\z/, 'invalid pinned commit');
my @root_row = lstat($literal);
need(@root_row && S_ISDIR($root_row[2]), 'source root must be a real directory');
my $root = abs_path($literal);
my @git_row = lstat("$root/.git");
need(@git_row && S_ISDIR($git_row[2]), 'root .git must be a real directory');
local $ENV{GIT_NO_REPLACE_OBJECTS} = '1';
local $ENV{GIT_NO_LAZY_FETCH} = '1';
local $ENV{GIT_OPTIONAL_LOCKS} = '0';
sub git_read {
    open(my $pipe, '-|', 'git', '-C', $root, @_) or die "git: $!\n";
    binmode($pipe);
    my $data = '';
    while (1) {
        my $n = read($pipe, my $chunk, 65536);
        need(defined($n), 'cannot read Git tree');
        last unless $n;
        $data .= $chunk;
        need(length($data) <= 16 * 1024 * 1024, 'Git tree exceeds source guard limit');
    }
    need(close($pipe), 'Git tree command failed');
    return $data;
}
sub check_head {
    need(git_read('rev-parse', '--verify', 'HEAD^{commit}') eq "$commit\n", 'HEAD differs from dependencies.lock.json');
}
check_head();
my $tree = git_read('ls-tree', '-rz', '--full-tree', $commit);
need(length($tree) && substr($tree, -1) eq "\0", 'empty or truncated Git tree');
my (%entries, %directories);
for my $entry (split(/\0/, $tree)) {
    need(scalar($entry =~ /\A(100644|100755|120000) blob ([0-9a-f]{40})\t(.+)\z/s), 'unsupported tree entry (including submodule)');
    my ($mode, $oid, $path) = ($1, $2, $3);
    need($path !~ m{\A/|(?:\A|/)\.{1,2}(?:/|\z)|(?:\A|/)\.git(?:/|\z)} && !exists($entries{$path}), 'invalid or duplicate tree path');
    $entries{$path} = [$mode, $oid];
    my $parent = $path;
    $directories{$parent} = 1 while $parent =~ s{/[^/]+\z}{};
}
sub inventory {
    my %found;
    my @pending = ('');
    while (@pending) {
        my $directory = pop @pending;
        opendir(my $dh, "$root/$directory") or die "opendir $directory: $!\n";
        local $! = 0;
        my @names = readdir($dh);
        my $read_error = 0 + $!;
        need(closedir($dh) && !$read_error, "cannot enumerate $directory");
        for my $name (@names) {
            next if $name eq '.' || $name eq '..' || ($directory eq '' && $name eq '.git');
            my $path = length($directory) ? "$directory/$name" : $name;
            my @s = lstat("$root/$path");
            need(scalar(@s), "cannot inspect $path");
            if (S_ISDIR($s[2])) { push @pending, $path; next; }
            need(exists($entries{$path}), "untracked or ignored source path: $path");
            need(S_ISREG($s[2]) || S_ISLNK($s[2]), "non-regular source path: $path");
            $found{$path} = 1;
        }
    }
    need(keys(%found) == keys(%entries), 'tracked source is missing');
    return join("\0", sort keys %found);
}
my $inventory = inventory();
sub row {
    my (@s) = @_;
    need(scalar(@s), 'missing file metadata');
    return join(':', @s[0,1,2,3,4,5,7,9,10]); # No atime; no nanosecond identity claim.
}
for my $path (sort keys %entries) {
    my ($mode, $oid) = @{$entries{$path}};
    my $file = "$root/$path";
    my @before = lstat($file);
    if ($mode eq '120000') {
        need(@before && S_ISLNK($before[2]), "symlink type changed: $path");
        my $target = readlink($file);
        need(defined($target) && sha1_hex('blob '.length($target)."\0".$target) eq $oid, "symlink bytes changed: $path");
        my $physical = abs_path($file);
        need(defined($physical) && index($physical, "$root/") == 0, "symlink escapes pinned source: $path");
        my $relative = substr($physical, length($root) + 1);
        need(exists($entries{$relative}) || exists($directories{$relative}), "symlink target is not pinned: $path");
        need(row(@before) eq row(lstat($file)) && readlink($file) eq $target, "symlink changed while read: $path");
        next;
    }
    need(@before && S_ISREG($before[2]) && (($before[2] & 0111) != 0) == ($mode eq '100755'), "file type/executable bit changed: $path");
    sysopen(my $fh, $file, O_RDONLY | O_NOFOLLOW | O_NONBLOCK) or die "open $path: $!\n";
    binmode($fh);
    my @opened = stat($fh);
    need(row(@before) eq row(@opened), "file changed before read: $path");
    my $hash = Digest::SHA->new(1);
    $hash->add('blob '.$opened[7]."\0");
    my $size = 0;
    while (1) {
        my $n = read($fh, my $chunk, 1024 * 1024);
        need(defined($n), "cannot read source: $path");
        last unless $n;
        $size += $n;
        need($size <= $opened[7], "source grew while read: $path");
        $hash->add($chunk);
    }
    need($size == $opened[7] && $hash->hexdigest eq $oid, "source bytes differ from pinned commit: $path");
    need(row(@opened) eq row(stat($fh)) && row(@opened) eq row(lstat($file)), "source changed while read: $path");
    close($fh) or die "close $path: $!\n";
}
need(inventory() eq $inventory, 'source inventory changed while read');
check_head();
need(row(@root_row) eq row(lstat($literal)), 'source root changed while read');
PERL
}
source_verified=0
finish() {
    status=$?
    trap - 0
    if [ "$source_verified" = 1 ]; then
        if source_guard; then :; else
            # Preserve a configure/build/install error even if its final fence fails.
            if [ "$status" = 0 ]; then status=1; fi
        fi
    fi
    exit "$status"
}
trap finish 0
source_guard
source_verified=1
# Keep dependency caches local; never upload build artifacts to upstream.
export HUNTER_ROOT="$deps/hunter"
# Older bundled fmt uses consteval constructs rejected by AppleClang 21.
# Apply through compiler flags so Hunter re-extraction cannot discard the fix.
fmt_flags=""
if [ "$(uname -s)" = Darwin ]; then
    fmt_flags="-DFMT_CONSTEVAL="
fi
set --
if [ "$source_only" = 1 ]; then
    set -- -DHUNTER_USE_CACHE_SERVERS=NO -DHUNTER_KEEP_PACKAGE_SOURCES=ON \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
fi
"$cmake_bin" -S "$source_dir" -B "$deps/libp2p-build" \
    -DCMAKE_TOOLCHAIN_FILE="$root/cmake/libp2p-toolchain.cmake" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$deps/install" \
    -DCMAKE_CXX_FLAGS="$fmt_flags" -DTESTING=OFF -DEXAMPLES=OFF -DCLANG_FORMAT=OFF -DHUNTER_RUN_UPLOAD=OFF "$@"
source_guard
"$cmake_bin" --build "$deps/libp2p-build" --parallel "${JOBS:-4}"
source_guard
"$cmake_bin" --install "$deps/libp2p-build"
