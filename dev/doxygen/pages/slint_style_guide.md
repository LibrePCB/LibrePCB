Slint UI Style Guide {#doc_slint_style_guide}
=============================================

[TOC]

This page describes the Slint UI coding style & conventions for LibrePCB
developers.


# Element ID Naming {#doc_slint_style_guide_id_naming}

Any functional UI elements should have an ID assigned as they might be
used for automated GUI testing sooner or later. Elements are considered as
functional if they are interactive (`Button`, `FocusScope` etc.) or
otherwise important for tests (e.g. a conditional `Image`, in contrast to
a static `Image`). Typical non-functional elements are `Rectangle`,
`VerticalLayout` etc, though sometimes you might give them an ID anyway.

IDs shall be `snake-case` and must be unique within a Slint component.
They shall start with the most generic term, then more specific terms, and
the last segment shall be a type specification of the element. Examples:

- `library-ta`
- `library-title-txt`
- `library-title-hint-txt`

If within a component there are multiple elements with the exact same function
(e.g. their visibility depend on window size), a number shall be added as the
last segment in their ID. Example: `donate-btn-1`, `donate-btn-2`


## Element ID Abbreviations {#doc_slint_style_guide_id_abbreviations}

For commonly used components, we use ID abbreviations to keep the code short.
Abbreviations shall be used only for the type specification in IDs, i.e.
the last segment of the snake-case IDs. The following abbreviations shall
be used for the given component types:

| Abbrev. | Component types |
|---------|-----------------|
| `btn`   | `Button` and other kinds of clickable component |
| `cbx`   | `ComboBox` |
| `dlg`   | `Dialog` |
| `edt`   | `TextEdit`, `TextInput` and other kinds of editing component |
| `fs`    | `FocusScope` |
| `img`   | `Image` |
| `item`  | Any kinds of list view or tree view item |
| `l`     | `VerticalLayout`, `HorizontalLayout`, `GridLayout` etc. |
| `r`     | `Rectangle` |
| `sw`    | `Switch`, `CheckBox` and other checkable componens, except buttons |
| `ta`    | `TouchArea` |
| `tt`    | `ToolTip` |
| `txt`   | `Text` |
| `view`  | `ListView`, `TreeView` and other kinds of data view component |

For components not listed here, a descriptive name shall be used
(e.g. `spinner` for `Spinner` components).


# Accessibility {#doc_slint_style_guide_accessibility}

Any element with accessibility function shall define the corresponding
[Accessibility Properties](https://docs.slint.dev/latest/docs/slint/reference/common/#accessibility-properties).

Note that various Slint components already set these properties by default,
e.g. the `Text` component. This is not always desired as sometimes `Text`
components are used to display content already made accessible by another
element. Or the `Image` component sets the accessibility role to `image` by
default, but in our case images are almost always just icons not relevant
to the user. In such cases, explicitly set the accessibility role to `none`.

Simplified example:

```Slint
export component MenuItem inherits TouchArea {
    // Accessibility
    accessible-role: button;
    accessible-checkable: true;
    accessible-checked: false;
    accessible-description: @tr("This does something.");
    accessible-enabled: true;
    accessible-label: @tr("Menu Item");
    accessible-action-default => {
        root.clicked();
    }

    icon-img := Image {
        source: @image-url("icon.svg");
        accessible-role: none;  // There's nothing interesting with this image!
    }

    text-txt := Text {
        text: @tr("Menu Item");
        accessible-role: none;  // Already handled by parent element!
    }

    tooltip-txt := Text {
        text: @tr("This does something.");
        accessible-role: none;  // Already handled by parent element!
    }
}
```
