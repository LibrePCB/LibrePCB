# Contributing Guidelines

## Notes

- **Before spending lots of time on something, ask for feedback on your idea first!**
- Please search issues and pull requests before adding something new to avoid duplicating efforts and conversations.
- Issues which are considered to be easy to solve are marked with the label `easy`:
  [Browse all easy issues](https://github.com/LibrePCB/LibrePCB/labels/easy).
  Some issues even have a mentor assigned:
  [Browse all mentored issues](https://github.com/LibrePCB/LibrePCB/labels/mentored).
- To contact us use either GitHub or join our [IRC](https://webchat.freenode.net/?channels=#librepcb)
  or [Telegram](https://telegram.me/LibrePCB_dev) chat (they are automatically synchronized).

## Getting Started

- Make sure you have a [GitHub account](https://github.com/signup/free).
- Open a new issue for your idea, assuming one does not already exist.
- Fork the repository on GitHub.
- Have a look at our [development resources](https://github.com/LibrePCB/LibrePCB/tree/master/dev),
  especially at the [Doxygen documentation](https://doxygen.librepcb.org/master/).
- When using QtCreator, import and use our [code style guide file](https://github.com/LibrePCB/LibrePCB/blob/master/dev/CodingStyle_QtCreator.xml).

## Making Changes

- Create a topic branch from where you want to base your work.
  - This is usually the master branch.
  - To quickly create a topic branch based on master:
    `git checkout -b my_contribution master`
  - Please avoid working directly on the `master` branch.
- Write code which follows our [code style guides](https://doxygen.librepcb.org/master/df/d24/doc_code_style_guide.html)
  and [.editorconfig settings](https://github.com/LibrePCB/LibrePCB/blob/master/.editorconfig).
- Make commits of logical units.
  - Make sure your commit messages are in the [proper format](http://chris.beams.io/posts/git-commit/):
```
ScopeGuardList: Fix crash when constructing with size

Default constructed std::function is empty and throws an
std::bad_function_call when being called.

Check if it is empty and use reserve() when constructing with size.

Fixes #62
```
- Make sure you have added the necessary tests for your changes.
- Run all tests to ensure nothing else was accidentally broken.
  - This is done by running the binary `./build/output/librepcb-unittests`
- If you like, feel free to add yourself to the
  [AUTHORS.md](https://github.com/LibrePCB/LibrePCB/blob/master/AUTHORS.md) file.

## Submitting Changes

- Push your changes to a topic branch in your fork of the repository.
- Submit a pull request to the repository in the LibrePCB organization.
- We will then check the pull request and give you feedback quickly.

## Other Contributions

This project welcomes non-code contributions, too! The following types of contributions are welcome:

- **Ideas**: Participate in an issue thread or start your own to have your voice heard.
- **Bugreports**: Found a bug in LibrePCB? Just open an issue and describe how to reproduce that bug.
- **Small Fixes**: Fix typos, clarify language, and generally improve the quality of the content.
- **Documentation**: Create/improve [documentations for users](https://github.com/LibrePCB/librepcb-doc)
  or developers of LibrePCB.
- **Translations**: Add/improve translations for LibrePCB using
  [Transifex](https://www.transifex.com/librepcb/librepcb-application/dashboard/).
  Follow [this guide](https://docs.transifex.com/getting-started/translators) to
  get started.
- **Website**: Improve our [website](http://librepcb.org) which is hosted in the repository [LibrePCB/LibrePCB.github.io](https://github.com/LibrePCB/LibrePCB.github.io).
- **Sharing**: Speak about LibrePCB with your friends and colleagues, or write about it in the internet!
- **Donations**: [Become a Patron](https://www.patreon.com/librepcb)
  or donate to the LibrePCB developers with Bitcoin:
  [1FiXZxoXe3px1nNuNygRb1NwcYr6U8AvG8](bitcoin:1FiXZxoXe3px1nNuNygRb1NwcYr6U8AvG8)

# Additional Resources

- [General GitHub documentation](https://help.github.com/)
- [GitHub pull request documentation](https://help.github.com/send-pull-requests/)
