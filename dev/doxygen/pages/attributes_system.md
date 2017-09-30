Attributes System {#doc_attributes_system}
==========================================

[TOC]

LibrePCB uses a sophisticated attributes system which makes it possible to
display dynamically determined information in schematics and boards, or to
provide additional information for the BOM.


# Example: Symbol {#doc_attributes_system_example_symbol}

Let's start with a short example. A symbol as defined in the library could look
like this:

![Library Symbol](library_symbol.png)

It contains various texts on different layers, and each of them will be replaced
by a dynamically determined text while the symbol is part of a schematic:

<table>
  <tr>
    <th>Text
    <th>Graphics Layer
    <th>In schematics substituted by following value
  <tr>
    <td>`#NAME`
    <td>`sym_names`
    <td>Designator of the symbol's component in a schematic (e.g. "R5")
  <tr>
    <td>`#VALUE`
    <td>`sym_values`
    <td>Most important attribute of the symbol's component
        (e.g. "100nF" for a capacitor, or "LM358N" for an OpAmp)<br>
        *Note: With which attribute's value `#VALUE` is substituted is
        controlled by the corresponding component, not by the symbol itself.*
  <tr>
    <td>`#PARTNUMBER`
    <td>`sym_attributes`
    <td>Part number of the linked device (e.g. "LM358N")<br>
        *Note: Such arbitrary attributes should normally not be added to
        symbols! Instead, the `#VALUE` attribute should be used in most cases.*
</table>

The `#NAME` and `#VALUE` are the most important attributes of a symbol, nearly
every symbol will have text items to display them in schematics. They also have
their own graphics layer, so it's possible to show or hide the names and values
of all symbols in a schematic by changing the visibility of these two layers.

For text items which display other attributes (e.g. `#PARTNUMBER`, `#RESISTANCE`
or `#FOOBAR`) the graphics layer `sym_attributes` should be used. There are no
more layers to further distinguish between different attributes.

Exactly the same system also applies to footprints, with the only exception that
different graphics layers are used (`top_names`, `bot_names`, `top_values`, ...).


# Built-In and User-Defined Attributes {#doc_attributes_system_builtin_userdefined}

There are two different kinds of attributes:

**Built-In Attributes:**
These attributes are automatically defined by the application and cannot be
removed. Some of these attributes values can still be modified by the user,
but some are fully defined by the application and are not editable.

**User-Defined Attributes:**
On some scopes (see next chapter), the user can also add arbitrary attributes in
addition to the built-in ones. They always have higher priority than the
built-in ones, so the user is (theoretically) able to override built-in
attributes if necessary.


# Scopes {#doc_attributes_system_scopes}

Attributes can be defined at various objects, with different scopes:

| Object of class                      | Scope                                 |
|--------------------------------------|---------------------------------------|
| librepcb::project::Project           | Whole project (global)                |
| librepcb::project::ComponentInstance | One component, its device and symbols |
| librepcb::project::Schematic         | One schematic page and its symbols    |
| librepcb::project::SI_Symbol         | One schematic symbol                  |
| librepcb::project::Board             | One board, its devices and components |
| librepcb::project::BI_Device         | One device and its package            |


# Lookup Order {#doc_attributes_system_lookup_order}

If the value of an attribute needs to be determined for one specific purpose
(for example to substitute a text in a symbol), the lookup is done in a specific
order. The red arrows of the following diagram show how this order is defined.
It also lists all built-in attributes (the ones starting with a '#') and on
which scopes it is possible to add user-defined attributes.

![Attributes System](attributes_system.png)


# Multiple Key Substitution {#doc_attributes_system_multiple_key_substitution}

Sometimes the `#VALUE` text in a symbol or footprint should be substituted by
the most exact attribute of a component or device. For example a microcontroller
symbol in the schematic should show the exact part number of the corresponding
device (Prio. 1). But if no part number is available, the device name should be
shown instead (Prio. 2). And if the component does not even have a device
assigned, the component name should be shown (Prio. 3). This table shows these
three cases for the [STM32F103C8T7TR](http://www.st.com/resource/en/datasheet/stm32f103c8.pdf):


| Prio. | Component Name | Device Name    | Device Part Number | Desired Substitution of `#VALUE` |
|-------|----------------|----------------|--------------------|----------------------------------|
| 1     | "STM32F103C"   | "STM32F103CxT" | "STM32F103C8T7TR"  | "STM32F103C8T7TR"                |
| 2     | "STM32F103C"   | "STM32F103CxT" | N/A                | "STM32F103CxT"                   |
| 3     | "STM32F103C"   | N/A            | N/A                | "STM32F103C"                     |

To get this "best-match-attribute" behavior, the `#VALUE` attribute of a
component can be set to the value "#PARTNUMBER|DEVICE|COMPONENT". So the '|'
character after a key means that a fallback key is following if the first key
is not set to a non-empty value. As soon as the first value of the specified
keys is not empty, all following keys are ignored.

Of course this system applies to arbitrary attributes, not only to `#VALUE`.


# Syntax {#doc_attributes_system_syntax}

## Allowed Characters {#doc_attributes_system_allowed_characters}

Attribute keys must only consist of the characters 0-9, a-z, A-Z and '_'.
The first invalid character after a '#' is considered as the end of the key
(e.g. in the string "#FOO.BAR", only "#FOO" will be substituted).

In addition, the character '|', which is used for
@ref doc_attributes_system_multiple_key_substitution and
@ref doc_attributes_system_manual_end, has special functionality inside or at
the end of key names. Outside of keys, this character does not have a special
functionality, so it will not be substituted and does not require escaping.

## Manual End of Key {#doc_attributes_system_manual_end}

If the end of a key needs to be specified manually (e.g. to only substitute
"#FOO|BAR" in the string "#FOO|BAR42"), the key(s) must be followed by "||"
(i.e. "#FOO|BAR||42").

## Escaping {#doc_attributes_system_escaping}

To use the '#' character in a text item without the need for attribute
replacement, the character needs to be escaped by a second '#'. For example the
text "##FOOBAR##" will be rendered as "#FOOBAR#" without any attribute
substitution.


# Visibility in Schematics and Boards {#doc_attributes_system_visibility}

Generally the user wants to select which attributes of a component should be
visible in the schematic and in the board. Sometimes schematics and boards
should even show different attributes of the same component, for example a
capacitor should show the value "100nF/50V" in the schematic, but only "100nF"
in the board because of the limited space.

By default, symbols and footprints show all the attributes which are contained
in a text item in their library elements (like `#NAME`, `#VALUE` and
`#PARTNUMBER` in the symbol above). But in schematics and boards, the user can
remove every single of these text items to hide specific attributes. Or to show
more attributes, new text items (with arbitrary values) can be added to symbols
and footprints. New text items are then added with some default properties
(layer, text size, position, ...) which the user needs to adjust afterwards.

**Anyway, adding attributes other than `#NAME` and `#VALUE` to library symbols
and footprints should be used very rarely to keep symbols as clean and simple as
possible. For the most important attributes, the `#VALUE` text item should be
used.**


# Example Use-Cases {#doc_attributes_system_example_usecases}

Here are some examples how the attribute system works for standard use-cases:

<table>
  <tr>
    <th>Use-Cases
    <th>Symbol /<br>Footprint
    <th>Component Attributes /<br>Device Attributes
    <th>Component Defaults / <br>Device Defaults
    <th>Component Values /<br>Device Values
    <th>Schematic /<br>Board
  <tr>
    <td rowspan="5">Resistor<br>Capacitor<br>Inductor
    <td rowspan="4">@image html resistor_symbol.png
    <td>#VALUE <td colspan="2">"#RESISTANCE #TOLERANCE #POWER"
    <td rowspan="4">@image html resistor_schematic.png
    <tr><td>#RESISTANCE <td><td>"10Ω"
    <tr><td>#TOLERANCE <td><td>"1%"
    <tr><td>#POWER <td><td>"5W"
  <tr>
    <td rowspan="1">@image html resistor_footprint.png
    <td>#VALUE <td colspan="2">"#RESISTANCE"
    <td rowspan="1">@image html resistor_board.png
  <tr>
    <td rowspan="2">Diode<br>Transistor<br>OpAmp
    <td rowspan="1">@image html diode_symbol.png
    <td>#VALUE <td colspan="2">"#PARTNUMBER|DEVICE"
    <td rowspan="1">@image html diode_schematic.png
  <tr>
    <td rowspan="1">@image html diode_footprint.png
    <td colspan="3">N/A
    <td rowspan="1">@image html diode_board.png
  <tr>
    <td rowspan="4">LED
    <td rowspan="2">@image html led_symbol.png
    <td>#VALUE <td colspan="2">\"#PARTNUMBER|DEVICE <br> #COLOR\"
    <td rowspan="2">@image html led_schematic.png
    <tr><td>#COLOR <td colspan="2">
  <tr>
    <td rowspan="2">@image html led_footprint.png
    <td>#VALUE <td colspan="2">"#COLOR"
    <td rowspan="2">@image html led_board.png
    <tr><td>#COLOR <td colspan="2">"RED"
  <tr>
    <td rowspan="2">Connector
    <td rowspan="1">@image html connector_symbol.png
    <td>#VALUE <td colspan="2">"#PARTNUMBER|DEVICE"
    <td rowspan="1">@image html connector_schematic.png
  <tr>
    <td rowspan="1">@image html connector_footprint.png
    <td colspan="3">N/A
    <td rowspan="1">@image html connector_board.png
  <tr>
    <td rowspan="2">Microcontroller
    <td rowspan="1">@image html microcontroller_symbol.png
    <td>#VALUE <td colspan="2">"#PARTNUMBER|DEVICE|COMPONENT"
    <td rowspan="1">@image html microcontroller_schematic.png
  <tr>
    <td rowspan="1">@image html microcontroller_footprint.png
    <td colspan="3">N/A
    <td rowspan="1">@image html microcontroller_board.png
  <tr>
    <td>Sheet Frame
    <td>@image html sheetframe_symbol.png
    <td colspan="3">N/A
    <td>@image html sheetframe_schematic.png
</table>


# Implementation {#doc_attributes_system_implementation}

Every class which provides some attributes need to inherit from
librepcb::AttributeProvider. Please read the documentation of
librepcb::AttributeProvider to see how this class works in detail.

The substitution of attributes in strings is done by the class
librepcb::AttributeSubstitutor.
