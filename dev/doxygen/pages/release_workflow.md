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
which are merged into the `master` branch as soon as they are finished. However,
to reduce the complexity of the development workflow, we only accept file format
changes on the `master` branch if we're working on the next major release! So
if the next release is planned to be a minor or bugfix release, no file format
changes shall be merged into `master`. In practice this means that for example
every 1-2 years there is a window of a few months where file format changes
are accepted.

Although it is a bit awkward to hold back new features affecting the file
format, this is much more efficient. We learned in the past that accepting
file format changes at any time causes a lot of overhead and complexity.

![Development Cycle](development_cycle_diagram.png)

Generally, releases shall be created directly on the `master` branch, i.e.
the release tag points to a commit on `master`. However, if an urgent bugfix
release is required but minor features implemented since the last release
shall not be released yet, it is possible to create a release branch based
on the last release with the new tag pointing to that release branch.

![Git Branches](release_workflow_diagram.png)

Note that no release other than the last one will be maintained. For example
after releasing LibrePCB 1.2.3, no bugfix releases will be created for any
previous release.


# Git Tags

For every officially released version we create a GPG signed Git tag on the
corresponding release commit. The tag name contains all three numbers of the
application's version number. Examples: "0.1.0-rc1", "0.1.0", "1.2.0", "1.2.1"


# Warning in Unstable Versions {#doc_release_workflow_warning}

Because (at least) the `master` branch is unstable while working on the next
major release, it's very dangerous for end users to work productively with
this version (it could break projects etc.!). So we should warn users in
that situation.

This is done by displaying warnings in the application's GUI. Their visibility
is controlled by the preprocessor define `FILE_FORMAT_STABLE`. It's set to `0`
(warning visible) while working on the next major release, otherwise to `1`
(warning disabled).


# Translations {#doc_release_workflow_translations}

We use [Transifex] to translate LibrePCB into other languages. The resource
`librepcb.ts` always contains the strings from the latest commit on the
`master` branch. Translations are automatically checked into the repository
[LibrePCB/librepcb-i18n](https://github.com/LibrePCB/librepcb-i18n).

The `librepcb-i18n` repository is included as a submodule in the main
repository. Because translations change very often (and thus we would have to
update the submodule very often), the submodule is not regularly updated
(to avoid commit spam). It gets updated only when preparing a new release
(important for packagers to get the correct translations).


# Changelog {#doc_release_workflow_changelog}

The changelog is not checked into the repository because it would probably lead
to confusion (add it to the `master` branch, release branches or even both?).
Instead, currently we use our blog to list the changes of every release.


# Deployment {#doc_release_workflow_deployment}

Releases built by ourselves (Windows builds, installers, AppImage etc.) are
published at https://download.librepcb.org, inclusive GPG signature. The website
https://librepcb.org contains direct download links to the corresponding files.

Packages of package managers (e.g. APT, Flatpak, homebrew, ...) are updated by
the corresponding package maintainers. Typically they clone the tag we released
and then build the application from sources. Thanks to the translations checked
into the release branches, package maintainers don't have to care about
downloading translations from [Transifex].


# Checklists {#doc_release_workflow_checklist}

## Start Working On A New Major Release

1. On `master`, add a commit to increment the application's major version and
   the file format version. In addition, set the define `FILE_FORMAT_STABLE`
   to `0`.

## Create Release

### Preparations

1. Only proceed if the file format is freezed and `FILE_FORMAT_STABLE`
   is set to `1`!
2. Make sure CI uses the latest Qt version. If necessary, update CI first and
   test the artifacts before proceeding with the release procedure.
3. Make sure all submodules point to persistent commits (commits could be
   garbage collected when rebasing a branch, this would break the checkout of
   older releases!). If necessary, add tags to the submodule repositories to
   avoid garbage collection of these commits.
4. Verify that translations are up-to-date, i.e. the auto-sync still works.

### Release

1. Create a new (temporary) branch, called e.g. `release-x.y.z`.
2. Update the `i18n` submodule and commit.
3. Update the define `LIBREPCB_APP_VERSION` in `CMakeLists.txt` (e.g.
   "0.1.0-rc1" or "0.1.1"). If needed, also update the installer version in
   `ci/build_windows_installer.sh` (e.g. "0.1.0-1") and commit everything.
   **Note: Always append a dash followed by a number to the installer version!**
4. Update the releases section in
   `share/metainfo/org.librepcb.LibrePCB.metainfo.xml`, commit and push.
5. Open a pull request (optional).
6. Check if CI is successful and test the artifacts generated by CI. Things
   to be checked explicitly:
     * Library download (check OpenSSL and QuaZip)
     * 3D viewer (check OpenGL)
     * Translations
7. If everything looks good, merge the branch into `master` (either by merging
   the PR or locally with merge commit or fast-forward).
8. Important: Only proceed with the following steps if you have a lot of time
   to do the rest of the release procedure in one chunk!
9. Publish the artifacts at https://download.librepcb.org/releases/.
10. Test the official releases again, this time including the installer.
11. If everything works properly, Add and push a new tag with the name equal
    to the value of `LIBREPCB_APP_VERSION` (e.g. "0.1.0-rc1"). Use the message
    "LibrePCB <LIBREPCB_APP_VERSION>". Keep in mind that the tag must be
    signed and the version number must always have three numbers (i.e. "0.1.0"
    instead of "0.1")!
12. Bump the application patch version and the installer version on `master`
    (e.g. to "1.2.3-unstable" after releasing "1.2.2"), commit and push.

### After The Release

1. Update website/documentation download links and publish blog post
   containing the changelog.
2. Add tag description to [GitHub Releases], including a link to the blog post.
3. Notify users through Patreon, Twitter, ...
4. Start updating distribution packages (Flatpak, Snap, Arch, NixOS, ...).


[semantic versioning]: https://semver.org/
[forward compatibility]: @ref doc_versioning_forward_compatibility
[Transifex]: https://transifex.com
[resource]: https://docs.transifex.com/projects/introduction
[GitHub Releases]: https://github.com/LibrePCB/LibrePCB/releases
