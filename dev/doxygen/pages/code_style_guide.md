Code Style Guide {#doc_code_style_guide}
========================================

[TOC]

This page describes the code style guide for LibrePCB developers.


# Format {#doc_code_style_guide_format}

- All files must have UNIX line endings (CI rejects Windows line endings).
- Always use spaces, never tabulators (CI rejects tabulators).
- Generally just follow the style of existing code and it will be fine :-)
- In the repository root there is a `.clang-format` file with the exact rules.
  You can use [clang-format](https://clang.llvm.org/docs/ClangFormat.html)
  (version 6.0.0) to automatically format files according these rules. Use the
  command `clang-format -style=file -i <FILE>` to format single files, or the
  script `./dev/format_code.sh` to format all files which are modified compared
  to the `master` branch (this avoids formatting of files other than the ones
  you modified).


# General {#doc_code_style_guide_general}

- Qt >= 5.2
- C++11
- Use strongly typed enums (`enum class`, C++11) whenever reasonable
- Use `nullptr` instead of `NULL` or `0`
- Use keywords like `const`, `constexpr`, `final`, `override`, ... whenever possible
- Use noexcept specifier in method declarations that never throw exceptions:

      void foo() noexcept;    // this method never throws exceptions

- Use Qt's smart pointers `QScopedPointer` and `QSharedPointer` for object ownership (instead of raw pointers)


# Header Files {#doc_code_style_guide_header_files}

- Use nearly always forward declarations when possible. Includes in header files are allowed:
    - for all Qt header files (like QtCore, QtWidgets,...)
    - when the header file uses other types by value (by reference/pointer --> use forward declaration)
    - when the header file highly depends on other types, even if only by reference/pointer


# Naming {#doc_code_style_guide_naming}

- File Names: Excatly same naming as classes, but all characters lowercase. Example: `ses_select.cpp`, `ses_select.h`
- Include guards: Use the namespace as prefix before the class name. Example: `#ifndef NAMESPACE_CLASSNAME_H`
- Namespaces: lowercase (with underline as separator if needed)
- Classes: UpperCamelCase (for common prefixes, use the underline as separator. Example: librepcb::project::editor::SES_Select)
- Interfaces: Like Classes, but with prefix "IF_". Example: librepcb::IF_GraphicsViewEventHandler
- Functions/Methods: lowerCamelCase
- Member Variables: lowerCamelCase, beginning with a 'm'. Example: `mSomeMemberVariable`
- Static Variables: lowerCamelCase, beginning with a 's'. Example: `sSomeStaticVariable`
- Function Parameters: lowerCamelCase
- Local Variables: lowerCamelCase
- Macros: uppercase, with underline as separator


# Comments {#doc_code_style_guide_comments}

- Use doxygen comments with '@' (not '\') as command character
- Methods documentation only in header files, not in source files


# Types {#doc_code_style_guide_types}

- If you need integer types with fixed width, us Qt's typedefs (for example `qint32`) instead of the
  types from `<stdint.h>` (for example `int32_t`).
- For floating point numbers, use Qt's typedef `qreal` instead of `float` or `double` whenever possible.
  This way the application should work also on ARM platforms with single precision FPU quite well.


# QObject {#doc_code_style_guide_qt_qobject}

- Inherit from `QObject` only when needed (for example if you only need `QObject::tr()`, see note to
  `Q_DECLARE_TR_FUNCTIONS()` in @ref doc_code_style_guide_qt_macros)
- Try to avoid using `QObject::connect()` with Qt's `SIGNAL()` and `SLOT()` macros. Use function
  addresses instead (whenever possible). This way, signals and slots are checked on compile-time
  instead of runtime.

      connect(&myTimer, SIGNAL(timeout()), this, SLOT(mySlot()));     // uses runtime check --> avoid this!
      connect(&myTimer, &QTimer::timeout, this, &MyClass::anyMethod); // uses compile-time check (and any method can be used as slots!) --> much better!
      connect(&mAutoSaveTimer, &QTimer::timeout, [this](){doSomething();});   // Using C++11 lambda functions as slots is also possible
                                                                              // --> can be very useful, but use it carefully (dangling pointers, ...)


# Qt Macros {#doc_code_style_guide_qt_macros}

- Do not use `Q_CHECK_PTR()` because it throws a `std::bad_alloc` exception which we will never
  catch (we only catch our own exception types). Use `Q_ASSERT()` instead.
- The macro `Q_DECLARE_TR_FUNCTIONS()` is very useful to use the translation method `QObject::tr()`
  (resp. `tr()`) also in classes which do not inherit from `QObject` (but do *NOT* use this macro in
  interface classes because of possible multiple inheritance --> use `QCoreApplication::translate()`
  instead in this case). See http://doc.qt.io/qt-5/qcoreapplication.html#Q_DECLARE_TR_FUNCTIONS
