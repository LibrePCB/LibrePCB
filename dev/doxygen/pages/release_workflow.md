Release Workflow {#doc_release_workflow}
========================================

[TOC]

This page describes how releases of LibrePCB are created and maintained.


# Application and File Format Versioning {#doc_release_workflow_versioning}

The file format of workspaces, libraries and projects is a very central part of
LibrePCB, see @ref doc_versioning for details how it's implemented and how we
handle forward/backward compatibility.

For the application we use version numbers according [semantic versioning]
\(MAJOR.MINOR.PATCH, e.g. "1.2.3"). So every increment of the major number means
a breaking change, which is a file format change in our case (we consider *any*
file format change as breaking because we don't provide [forward compatibility]
at all).

As the file format also needs a version number, we just use the application's
major version as the file format version. For example the application version
"1.2" has the file format "1", and "13.37.42" has file format "13". Please note
the special case of 0.x versions where the second number denotes the major
version, so the application version "0.1.2" has file format "0.1". This way, the
file format can always be determined by the application's version number.

Given that versioning system, it's clear that any application with the same
major version uses exactly the same file format. And for developers it's clear
that when changing something in the file format, the application's major version
*must* be incremented.


# Preprocessor Defines {#doc_release_workflow_defines}

The application version is compiled into the binary with the preprocessor define
`APP_VERSION` and the file format version with the define `FILE_FORMAT_VERSION`.
Whether the file format is stable or not is controlled by the define
`FILE_FORMAT_STABLE`. See following chapters for details.


# Git Branches {#doc_release_workflow_branches}

The continuous development happens on feature/bugfix branches (or pull requests),
which are merged into the `master` branch as soon as they are finished. As new
features sometimes also introduce file format changes, the `master` branch must
always be considered to contain the (unstable) *next* major version of the
application. For example if the last stable release was version "2.3.4", the
`master` must immediately be considered as version "3.0.0-unstable".

This also means that on the `master` branch we cannot provide updates for
already released versions. So we have to provide such updates in release
branches. For every major version we create a release branch, which is then used
for *any* update for that major version. For example on the branch `release/2`
we would provide the releases "2.0", "2.0.1" (bugfix) and "2.1.0" (new feature).

![Git Branches](release_workflow_diagram.png)

Features and bugfixes which do not affect the file format may be cherry-picked
from `master` to release branches. This way we can publish updates for the last
officially released application version while the next major version is still in
development. Generally we do *not* maintain older releases than the last stable
release because this would be too much effort. So for example as soon as the
version "0.2.0" is released, the version "0.1.x" is no longer maintained.


# Git Tags

For every officially released version we create a GPG signed Git tag on the
corresponding release branch. The tag name contains all three numbers of the
application's version number. Examples: "0.1.0-rc1", "0.1.0", "1.2.0", "1.2.1"


# Warning in Unstable Versions {#doc_release_workflow_warning}

Because (at least) the `master` branch is always unstable, it's very dangerous
for end users to work productively with this version (it could break projects
etc.!). So we should warn users in that situation.

This is done by displaying warnings in the application's GUI. Their visibility
is controlled by the preprocessor define `FILE_FORMAT_STABLE`. It's set to `0`
(warning visible) on all branches, except on release branches where it's set to
`1` (warning disabled). As release branches always have a stable file format, it
should be safe to use *any* state of a release branch (even if not officially
released).


# Translations {#doc_release_workflow_translations}

We use [Transifex] to translate LibrePCB into other languages. As Transifex
doesn't have support for branches, we use a separate [resource] for each major
application version. The translations for the `master` branch are contained in
the resource `librepcb.ts`, while translations for releases are contained in
resources like `librepcb-0.1.ts`.

Translations are automatically checked into the repository
[LibrePCB/librepcb-i18n](https://github.com/LibrePCB/librepcb-i18n). The
`master` branch contains translations of the resource `librepcb.ts`, while
translations of resources for releases are available on branches for the
corresponsing releases (e.g. `release/0.1` for `librepcb-0.1.ts`).

The `librepcb-i18n` repository is included as a submodule in the main
repository. On the `master` branch, the submodule points to a commit which does
not contain any translation files because translations change very often, so we
would have to update the submodule very often too (avoid commit spam). But on
release branches, the submodule points to the translations for the corresponding
release so they are always available when building official releases (important
for packagers).


# Changelog {#doc_release_workflow_changelog}

The changelog is not checked into the repository because it would probably lead
to confusion (add it to the `master` branch, release branches or even both?).
Instead, we use the [GitHub Releases] page to list the changes of every release
(resp. tag).


# Deployment {#doc_release_workflow_deployment}

Releases built by ourselves (Windows builds, installers, AppImage etc.) are
published at https://download.librepcb.org, inclusive GPG signature. The website
https://librepcb.org contains direct download links to the corresponding files.

Packages of package managers (e.g. APT, Flatpak, homebrew, ...) are updated by
the corresponding package maintainers. Typically they clone the tag we released
and then build the application from sources. Thanks to the translations checked
into the release branches, package maintainers don't have to care about
downloading translations from [Transifex].


# Release Checklist {#doc_release_workflow_checklist}

## Prepare New Major Release

1. Review the file format of the current `master` branch very carefully. We must
   be sure that the file format is completely stable *before* creating a release
   branch.
2. On [Transifex], create a new [resource] by copying `librepcb.ts`.
3. Create a new release branch (e.g. `release/0.1`) from the current `master`.
4. On `master`, add a commit to increment the application's major version and
   the file format version.
5. On the release branch, set the define `FILE_FORMAT_STABLE` to `1` and commit.
6. On the release branch, change resource in `.tx/config` to the corresponding
   release (e.g. 'librepcb-0.1.ts') and commit.

## Create Release

1. Cherry-pick relevant commits from `master`.
2. Update the `i18n` submodule and commit.
3. Update the define `APP_VERSION` (e.g. "0.1.0-rc1" or "0.1.1") and commit.
4. Update the installer version in `ci/build.sh` (e.g. "0.1.0-1"), commit and
   push. Note: Always append a dash followed by a number!
5. Check if CI is successful and test the artifacts generated by CI.
6. Publish the artifacts at https://download.librepcb.org/releases/.
7. Test the official releases again, this time including the installer.
8. Make sure all submodules point to persistent commits (commits could be
   garbage collected when rebasing a branch, this would break the checkout of
   older releases!). If necessary, add tags to the submodule repositories to
   avoid garbage collection of these commits.
9. If everything works properly, add and push a new tag with the name equal to
   the value of `APP_VERSION` (e.g. "0.1.0-rc1"). Keep in mind that the tag
   must be signed and it's name must always have three numbers (i.e. "0.1.0"
   instead of "0.1")!
10. Bump application and installer version on `master`.
11. Update website/documentation download links and publish blog post
    containing the changelog.
12. Add tag description to [GitHub Releases], including a link to the blog post.
13. Notify users through Patreon, Twitter, ...
14. Start updating distribution packages (Flatpak, Arch, NixOS, ...).


[semantic versioning]: https://semver.org/
[forward compatibility]: @ref doc_versioning_forward_compatibility
[Transifex]: https://transifex.com
[resource]: https://docs.transifex.com/projects/introduction
[GitHub Releases]: https://github.com/LibrePCB/LibrePCB/releases
