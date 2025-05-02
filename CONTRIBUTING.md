# Contributing Guidelines

## Ways to Contribute

Note that this file only contains a guide for **code contributions**. However,
there are many other ways how to contribute to the LibrePCB project, see
**[librepcb.org/contribute](https://librepcb.org/contribute/)** for details.

## Notes

- **Before spending lots of time on something, please ask for feedback on
  your idea first!** Also note that we do not accept major changes
  (e.g. changes affecting the file format) at any time, see details in our
  [development workflow documentation](https://developers.librepcb.org/da/dbc/doc_release_workflow.html#doc_release_workflow_branches).
- Please search issues and pull requests before adding something new to avoid duplicating efforts and conversations.
- We use some labels to mark issues as suitable for contributors, check those
  out to find something to work on:
  - [`help wanted`](https://github.com/LibrePCB/LibrePCB/labels/help%20wanted)
  - [`easy`](https://github.com/LibrePCB/LibrePCB/labels/easy)
- To contact us, use one of the options listed at https://librepcb.org/help/.

## Getting Started

- Make sure you have a [GitHub account](https://github.com/signup/free).
- Open a new issue for your idea, assuming one does not already exist.
- Fork the repository on GitHub.
- Have a look at our
  [development resources](https://github.com/LibrePCB/LibrePCB/tree/master/dev),
  especially at the [developers documentation](https://developers.librepcb.org/).
- When using QtCreator, import and use our
  [code style guide file](https://github.com/LibrePCB/LibrePCB/blob/master/dev/CodingStyle_QtCreator.xml).

## Making Changes

- Create a topic branch from where you want to base your work.
  - This is usually the master branch.
  - To quickly create a topic branch based on master:
    `git checkout -b my_contribution master`
  - Please avoid working directly on the `master` branch.
- Write code which follows our
  [code style guides](https://developers.librepcb.org/df/d24/doc_code_style_guide.html)
  and [.editorconfig settings](https://github.com/LibrePCB/LibrePCB/blob/master/.editorconfig).
  - You can use [clang-format](https://clang.llvm.org/docs/ClangFormat.html) to
    automatically format the code. To format all files at once, just run the
    script [`./dev/format_code.sh`](dev/format_code.sh).
- Make commits of logical units.
  - Make sure your commit messages are in the
    [proper format](http://chris.beams.io/posts/git-commit/):
    ```
    ScopeGuardList: Fix crash when constructing with size

    Default constructed std::function is empty and throws an
    std::bad_function_call when being called.

    Check if it is empty and use reserve() when constructing with size.

    Fixes #62
    ```
- Make sure you have added the necessary tests for your changes.
- Run all tests to ensure nothing else was accidentally broken.
  - This is done by running the binary
    `./build/tests/unittests/librepcb-unittests`.
- If you like, feel free to add yourself to the
  [AUTHORS.md](https://github.com/LibrePCB/LibrePCB/blob/master/AUTHORS.md)
  file.

## Submitting Changes

- Push your changes to a topic branch in your fork of the repository.
- Submit a pull request to the repository in the LibrePCB organization.
- We will then check the pull request and give you feedback quickly.

Please also take a look at our
[Pull Request Guidelines](https://developers.librepcb.org/df/d30/doc_developers.html#doc_developers_pullrequests).

## Additional Resources

- [General GitHub documentation](https://help.github.com/)
- [GitHub pull request documentation](https://help.github.com/send-pull-requests/)
