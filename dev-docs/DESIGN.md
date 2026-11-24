---
version: alpha
name: Woday
description: Visual design system derived from Woday’s public homepage and changelog.
colors:
  canvas: "#000000"
  surface: "#FFFFFF"
  on-canvas: "#FFFFFF"
  on-surface: "#000000"
  body-muted: "#757575"
  label-muted: "#666666"
  chip-surface: "#F5F5F5"
  dark-disclosure: "rgba(255, 255, 255, 0.05)"
  dark-secondary-action: "rgba(255, 255, 255, 0.10)"
typography:
  hero:
    fontFamily: Inter Display
    fontSize: 80px
    fontWeight: 700
    lineHeight: 1.05
    letterSpacing: -0.04em
  section-title:
    fontFamily: Inter Display
    fontSize: 60px
    fontWeight: 700
    lineHeight: 1.08
    letterSpacing: -0.04em
  statement:
    fontFamily: Inter
    fontSize: 30px
    fontWeight: 700
    lineHeight: 1.4
    letterSpacing: -0.04em
  update-title:
    fontFamily: Inter Display
    fontSize: 26px
    fontWeight: 700
    lineHeight: 1.3
    letterSpacing: -0.02em
  plan-title:
    fontFamily: Inter Display
    fontSize: 24px
    fontWeight: 700
    lineHeight: 1.3
    letterSpacing: -0.02em
  body:
    fontFamily: Inter Display
    fontSize: 20px
    fontWeight: 400
    lineHeight: 1.6
  eyebrow:
    fontFamily: Inter Display
    fontSize: 20px
    fontWeight: 600
    lineHeight: 1.4
    letterSpacing: -0.02em
  disclosure-title:
    fontFamily: Inter
    fontSize: 18px
    fontWeight: 700
    lineHeight: 1.4
    letterSpacing: -0.04em
  action:
    fontFamily: Inter Display
    fontSize: 16px
    fontWeight: 600
    lineHeight: 1.4
  nav-action:
    fontFamily: Inter Display
    fontSize: 14px
    fontWeight: 600
    lineHeight: 1.4
  chip:
    fontFamily: Inter Display
    fontSize: 16px
    fontWeight: 600
    lineHeight: 1.4
    letterSpacing: -0.02em
  meta:
    fontFamily: Inter Display
    fontSize: 14px
    fontWeight: 400
    lineHeight: 1.6
  price:
    fontFamily: Inter Display
    fontSize: 80px
    fontWeight: 700
    lineHeight: 1
    letterSpacing: -0.04em
rounded:
  disclosure: 20px
  compact-panel: 30px
  action: 30px
  panel: 60px
  chip: 120px
spacing:
  compact-gap: 10px
  text-gap: 20px
  phone-gutter: 16px
  desktop-gutter: 30px
  phone-panel-inline: 24px
  phone-panel-block: 30px
  tablet-panel-inset: 40px
  desktop-panel-inset: 60px
  desktop-panel-gap: 60px
  wide-section-inset: 100px
  content-max: 1200px
components:
  button-primary:
    backgroundColor: "{colors.canvas}"
    textColor: "{colors.on-canvas}"
    typography: "{typography.action}"
    rounded: "{rounded.action}"
    height: 52px
  button-download:
    backgroundColor: "{colors.surface}"
    textColor: "{colors.on-surface}"
    typography: "{typography.nav-action}"
    rounded: "{rounded.action}"
    height: 40px
  button-secondary-on-dark:
    backgroundColor: "{colors.dark-secondary-action}"
    textColor: "{colors.on-canvas}"
    typography: "{typography.action}"
    rounded: "{rounded.action}"
    height: 52px
  feature-panel:
    backgroundColor: "{colors.surface}"
    textColor: "{colors.on-surface}"
    rounded: "{rounded.panel}"
    padding: "{spacing.desktop-panel-inset}"
    width: "{spacing.content-max}"
  feature-chip:
    backgroundColor: "{colors.chip-surface}"
    textColor: "{colors.label-muted}"
    typography: "{typography.chip}"
    rounded: "{rounded.chip}"
  pricing-card:
    backgroundColor: "{colors.surface}"
    textColor: "{colors.on-surface}"
    rounded: "{rounded.compact-panel}"
    width: 400px
    padding: 40px
  faq-row:
    backgroundColor: "{colors.dark-disclosure}"
    textColor: "{colors.on-canvas}"
    typography: "{typography.disclosure-title}"
    rounded: "{rounded.disclosure}"
omitted:
  - section: motion
    reason: Interaction behaviors were observed, but exact transition timings were unverified.
---

# Woday Design System

## Overview

Woday presents its Mac planner on a black canvas with oversized white content panels. Large, tightly tracked headlines explain one benefit at a time. Native product screenshots show how calendar events, tasks, notes, and reminders fit together. The bright, furry W mascot adds personality through the app icon and occasional illustrations.

This reference covers the public homepage and changelog as observed on November 23, 2026. Token values come from rendered desktop styles or the site’s CSS. The tablet and phone rules were read from CSS. Visual inspection was at desktop width. Token names and usage guidance are editorial. [Homepage](https://www.woday.app/) · [Changelog](https://www.woday.app/updates)

## Colors

| Role | Value | Application |
| --- | --- | --- |
| Canvas and primary ink | `#000000` | Page shell, navigation, footer, light-panel headings, and primary CTA fill |
| Surface and inverse ink | `#FFFFFF` | Large panels, pricing card, dark-section headings, and inverse CTA fill |
| Muted body | `#757575` | Descriptions and metadata, with solid black or white behind the text |
| Muted label | `#666666` | Eyebrows and feature-chip labels on light surfaces |
| Soft light fill | `#F5F5F5` | Feature chips and light inset surfaces |
| Dark overlays | White at 5% or 10% | FAQ rows and secondary action backgrounds, respectively, over black |

Saturated red-orange comes from the mascot artwork. Gold stars mark social proof. Calendar labels, status dots, and other small colors belong to the product imagery. Navigation, headings, and purchase buttons use black, white, and gray.

## Typography

Use Inter Display for display headings, body text, navigation, labels, and actions. Use Inter for the bold editorial statements on black and for FAQ questions. Headlines use weight 700 and tight tracking. Supporting copy uses weight 400 with a more open line height. Feature eyebrows and button labels use weight 600 and sentence case.

| Role | Desktop | Tablet | Phone | Line height |
| --- | ---: | ---: | ---: | ---: |
| Hero and changelog headline | 80 px | 64 px | 40 px | 1.05 |
| Main section title | 60 px | 44 px | 32 px | 1.08 |
| Editorial statement | 30 px | 24 px | 19 px | 1.4 |
| Lead paragraph | 20 px | 18 px | 17 px | 1.6 |
| Feature eyebrow | 20 px | 16 px | 16 px | 1.4 |

Desktop detail sizes include 26 px update titles, 24 px pricing-card titles, 18 px FAQ questions, 16 px navigation and feature chips, and 14 px metadata. Use the visual roles independently of HTML heading rank. Give each page a single descriptive primary heading and use nested sections for the content below it.

## Layout

| Viewport | Main panel treatment | Content behavior |
| --- | --- | --- |
| Desktop, 1200 px and wider | Centered shell capped at 1200 px. Most feature panels have a 60 px inset and radius. | Side-by-side features use a roughly 400 px copy column and a 40 px gap. The FAQ puts its introduction beside the question list. |
| Tablet, 810–1199 px | Most feature panels reduce their inset to 40 px. | Calendar and Notes panels size to their content. The FAQ column gap reduces to 40 px. |
| Phone, below 810 px | Hero outer gutter falls to 16 px. Main white panels use 30 px corners and typically 30 px vertical by 24 px horizontal padding. | Side-by-side feature panels and the FAQ stack. Panel contents usually use 30 px gaps. Use the compact menu treatment and let chip groups wrap. |

The desktop page shell uses 30 px side gutters before its width cap. Keep the hero text centered within 730 px, the first feature overview within 700 px, and the closing CTA text within 600 px. The hero has 60 px top padding and ends with a product image clipped by its outer panel. The homepage uses roughly 60 px gaps between major feature panels and 20 px between related text elements. The desktop navigation is fixed at the top, with an 80 px header band.

### Homepage sequence

| Section | Composition |
| --- | --- |
| Hero | App icon, eyebrow, headline, short product description, stars and user count, Mac CTA, compatibility and purchase notes, wide app preview |
| Proof and feature overview | Award marks and a centered statement on black, followed by native app icons and a short iCloud note. The next white panel pairs overview copy and chips with a product preview. |
| Feature panels | Calendar and Notes use copy on the left and a cropped product image on the right. AI and Tasks place a wide image below the text. Quick Launcher places its interface image to the left of the desktop copy. |
| Pricing | Centered heading and purchase terms on black, followed by one 400 px white plan card. |
| Closing CTA | White panel with centered copy, a black CTA, and large product and mascot imagery. |
| FAQ, press, and footer | Dark FAQ with intro and mascot beside disclosures, followed by press badges and columns for brand, site links, support, and other products. |

### Changelog

Use the shared black header and footer around one large white panel. Center the opening copy and cap the release list at 760 px on desktop. Release rows use a date and version chip in the narrow left column, with category, title, summary, and a text link with an arrow in the wider right column. Faint horizontal rules and 40 px vertical row padding separate releases. [Changelog](https://www.woday.app/updates)

## Elevation & Depth

Most surfaces are flat. Black gutters and the edge of each white panel create separation without a drop shadow. Product-window borders, subtle image shadows, and mascot overlaps provide depth within sections. Use real Woday screenshots for feature illustrations. Keep navigation and body text in HTML, and treat screenshot annotations as supporting detail.

## Shapes

Use 60 px radii for the main desktop white panels, 30 px for the pricing card and narrow-screen panels, and 20 px for FAQ items. Actions and chips are pill-shaped. Keep the full rounded-square app icon in the header and hero. Use cutout mascot artwork as a supporting illustration, such as the sunglasses variant beside Calendar or the glasses variant in the FAQ. Use the Apple glyph with Mac download actions.

## Motion & Micro-interaction

Homepage navigation jumps to section anchors. FAQ rows toggle between collapsed and expanded states, revealing answers in place. Scroll entrances and a blue-glow cursor decoration were visible on desktop. When extending the site, preserve usable anchor offsets under the fixed header and provide a reduced-motion treatment for decorative effects.

## Voice & Content Strategy

Pair a short, outcome-led headline with a more specific label or supporting sentence. Use concrete verbs such as plan, capture, and organize. Reserve chips for scannable product capabilities. Keep platform, privacy, compatibility, and purchase terms close to the action they inform. Treat prices, release versions, promotional terms, and user counts as content that can change independently of the design.

## Components

| Component | Usage |
| --- | --- |
| Navigation | Black header, 40 px app icon beside a white wordmark, centered section links on desktop, and white Download pill at the right. The link spacing is about 20 px. |
| Primary Mac CTA | Black pill with white 16 px semibold text, a leading Apple glyph, 10 px gap, and 15 × 20 px vertical and horizontal padding. The navbar variant reverses the colors and uses 10 × 15 px padding with 14 px text. |
| Feature chip | Light gray pill with a muted label and optional leading icon. Use 8 × 16 px padding and 10 px gaps between chips. Keep descriptive chips non-interactive. |
| Feature panel | White surface with a gray eyebrow, bold black title, gray body, and chips. Give the relevant product screenshot the remaining space. Scale or crop it deliberately at the panel boundary. |
| Pricing card | White card with a 40 px inset, brand row, large current price, smaller struck-through former price when applicable, a soft offer inset, checklist, and full-width purchase CTA. The site’s other Mac CTAs lead to pricing. The purchase CTA in this card leads to checkout. |
| FAQ | Rounded, low-opacity white rows on black. Each row has a leading plus affordance, bold white question, and room to reveal the answer. Use a semantic button with an exposed expanded state, a visible focus treatment, and a programmatic relationship to its answer. |
| Release row | Date and version chip form the metadata group. Category, title, summary, and read link form the content group. Keep the row and its destination a single clear keyboard stop. |

## Do's and Don'ts

- Keep the page canvas black and the primary marketing panels white. Use the mascot and product imagery to carry saturated color.
- Keep headings compact and bold. Leave enough space around copy and screenshots for each section’s hierarchy to read clearly.
- Preserve the app screenshots’ proportions. Add concise alternative text when a screenshot supplies information beyond the adjacent copy.
- Avoid heavy borders, gradients, or shadows on the page shell and feature panels.
- Avoid turning feature chips or decorative stars into controls. Give actionable pills and disclosures visible focus states.
