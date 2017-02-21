Repository Specification {#doc_repository}
==========================================

[TOC]

This is the documentation of a LibrePCB repository. Such a repository is represented by an instance
of the class librepcb::Repository.


# Abstract

A repository is a server which holds a list of available libraries. The libraries must be hosted
somewhere as Zip files, for example on the Releases page of a GitHub repository. The library manager
retrieves the library list from the repositories to display the available libraries. If the user
wants to install such a library, the library manager just downloads these Zip files and extracts
them into the [remote libraries directory](@ref doc_workspace_remote_dir) in the @ref workspace.
After an automatically triggered rescan of the @ref doc_workspace_library_database, the library is
ready to use.


# Repository API

Every server which implements a specific API can be used as a LibrePCB repsository. Take a look at
[our own repository](https://github.com/LibrePCB/librepcb-api-static) to see how it looks like.


# Workspace Settings

In the workspace settings, the user can specify a list of URLs which are then used as repositories.
The default value is one single URL, namely LibrePCB's official repository [api.librepcb.org].


# Library Deployment in Companies

This system allows companies to manage and maintain their own libraries in one central repository,
from where each client fetches new libraries and updates. There is no need to manually deploy
libraries or to host them on slow and unreliable network shares.


[api.librepcb.org]: http://api.librepcb.org "Official LibrePCB Repository"
