# open-woday

An open-source, non-commercial desktop planner that brings the day-planning
experience of the upstream app to desktop users who cannot run it.

## Language

**Upstream app**:
The proprietary, macOS-only product "Woday" whose public behaviour this project
reimplements. Its documentation and brand assets are reference material, never
part of our shipped product.
_Avoid_: the original app, Woday (when referring to our implementation)

**open-woday**:
The name of this project and of its shipped desktop app — a descriptive,
unaffiliated FOSS alternative to the upstream app. It may appear as the product
name, but is never presented as official or endorsed.
_Avoid_: Woday, Wodie, "Woday for Linux", "the Woday port"

**Target user**:
A desktop user who cannot run the upstream app because it requires macOS.
_Avoid_: other users, the rest, non-Mac people

**Reference UI**:
The look and layout of the upstream app's own windows — a dark sidebar, white
rounded content panels and a right-hand rail. Distinct from the upstream app's
marketing website, whose styling is a separate reference and is not our UI spec.
_Avoid_: the design system, the website design

## Product domain

**Item**:
The single thing a user creates in open-woday. Everything — a Task, a Note or
an Event — is one Item of a particular kind.
_Avoid_: entry, record, object, card

**Task**:
An Item that can be done. It may carry a due date, a priority, Tags and
subtasks.
_Avoid_: to-do (that is a Block), reminder, action

**Note**:
An Item made of Blocks, used for writing. A Task or an Event can also own one.
_Avoid_: document, page, doc

**Block**:
One line of a Note's content. A to-do Block is also a real Task.
_Avoid_: node, paragraph, chunk

**Event**:
An Item that happens at a time, with a start and an end, or spanning a whole
day.
_Avoid_: appointment, meeting, calendar entry

**Tag**:
A label attachable to any Item, crossing Folders.
_Avoid_: label, category, keyword

**Folder**:
The single optional place an Item is filed.
_Avoid_: project, notebook, collection

**Inbox**:
The set of Items with no due date and no Folder — the "not dealt with yet"
pile.
_Avoid_: inbox folder, unsorted, catch-all

**Day note**:
The editable Note belonging to a single day. It holds that day's Tasks and
Events as lines and is what the Today page shows.
_Avoid_: daily note, journal, today page, diary
