---
name: ui-ux-pro-max
description: UI/UX design and review guidance for layouts, components, typography, color, spacing, motion, accessibility, responsiveness, and visual polish. Use when designing or evaluating interfaces, landing pages, dashboards, app screens, component libraries, or frontend UI code.
---

# UI/UX Pro Max

## Overview

Use this skill to make UI work feel intentional instead of generic. Favor strong hierarchy, clear spacing, readable type, deliberate color, and accessible interaction states.

## Workflow

1. Identify the surface, audience, and platform.
2. Choose one clear visual direction before editing anything.
3. Check the existing design system first and reuse its tokens, components, and spacing scales.
4. Strengthen hierarchy with layout, type, contrast, and grouping before adding decoration.
5. Add motion only when it clarifies state or focus.
6. Verify responsive behavior, keyboard access, touch targets, empty states, loading states, and error states.

## Quality Bar

- Prefer purposeful layouts over centered-card defaults.
- Prefer specific typography choices over system fallback stacks when the product allows it.
- Use a limited, coherent palette with reliable contrast.
- Keep spacing rhythm consistent across sections and components.
- Make primary actions obvious and secondary actions quieter.
- Show state clearly with hover, focus, pressed, disabled, and selected styles.
- Treat accessibility as a design constraint, not a final pass.

## Red Flags

- Flat gray-on-white pages with no hierarchy.
- Tiny touch targets or hover-only affordances.
- Inconsistent radius, shadow, or spacing tokens.
- Overused gradients, glow, glass, or emoji decoration.
- Hardcoded magic numbers when a design token already exists.
- Missing loading, empty, or error states.

## When Reviewing Or Implementing

- If the user asks for a review, call out visual hierarchy, accessibility, consistency, and responsiveness first.
- If the user asks for implementation, translate the design into concrete component structure and style decisions.
- If the stack already has a design system, follow it before inventing new patterns.
