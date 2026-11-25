# GooPad design language

GooPad should feel calm, approachable and local-first. The device and Studio
share a restrained neutral foundation, rounded surfaces, clear typography and
one blue primary action. User-selected tile colors, shapes, images and themes
remain their own; the product chrome must not override them.

## Principles

- Put the user's tiles first. Show the preview before secondary layout controls;
  reveal detailed controls progressively without removing capabilities.
- Use a small hierarchy: a prominent page title, concise section titles, then
  muted explanatory text. Describe actions in plain language.
- Use generous spacing, quiet borders and modest shadows rather than decorative
  effects. Reserve saturated color for primary actions and meaningful status.
- Keep keyboard focus visible, touch targets comfortable, status messages
  readable and reduced-motion preferences respected. Never rely on color alone.
- Adapt the same surface and typography rules to narrow screens, website
  themes and the device's 480-pixel display.

## Core tokens

The default dark Studio uses background `#101114`, surface `#1B1D22`, border
`#393C45`, primary text `#F5F5F7`, secondary text `#B0B3BD` and action blue
`#2969DB`. Light and alternate site themes provide their own accessible
counterparts. Surfaces use 14–20 px corners and controls use 10–14 px corners.
The Studio uses native system UI fonts; tile fonts are independently chosen.

The device keeps the top edge empty and opens its menu with a downward swipe.
The grid uses quiet outlined empty spaces and a pill-shaped current-page marker.
Every configured tile offers Classic, Modern and Layered card styles. Classic
uses the centered layout; Modern arranges large rectangular action tiles with
the icon above a left-aligned caption and gives large rectangular widgets a
heading. Layered adds caption surfaces and icon medallions to the Modern
layout, or inset widget surfaces when a heading does not fit. Small or curved
tiles use a simpler centered layout, so Classic and Modern may look alike.
Full-bleed GIFs cover the card surface and do not receive layered overlays.
Explicit gradients, colors, shapes, outlines and text alignment remain in
control. Existing tiles without an explicit style use Modern; newly created
tiles explicitly use Classic. Classic and Modern do not receive the Layered
treatment. The Studio preview follows these choices.
