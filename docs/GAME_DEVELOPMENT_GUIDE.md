# Game Development Guide

This document is the maintained design baseline for GSE Project. It records decisions established with the user and guides future Codex work. Explicit later user decisions take precedence and should be reflected here.

## 1. Fixed pillars

These are the project's default constraints:

- **Format:** single-player open-world RPG.
- **Presentation:** quarter-view 2.5D.
- **Setting:** a fictional modern city where ordinary life overlaps a hidden supernatural layer.
- **Tone:** warm, colorful, and familiar on the surface; darkness is glimpsed through small inconsistencies, absence, and memory loss.
- **Story shape:** the protagonist continues an ordinary life, encounters increasingly undeniable anomalies, gains a personal goal, and chooses what kind of everyday life should remain.
- **Primary theme:** recognizing the value of ordinary days, relationships, places, and memories before or through losing them.
- **Emotional center:** attachment rather than fear. The player should care about the city before its hidden cost is fully revealed.

### Theme statement

> An ordinary day is not a day when nothing special happens. It is a day when the things we value are still beside us.

The game should not suggest that suffering is inherently good. Its position is that grief and regret can be evidence of meaningful relationships, and that erasing pain can also erase people, places, and parts of a life.

## 2. Player experience

The player inhabits one protagonist and forms a personal history with the city. There are no multiplayer, cooperative, competitive, live-service, or shared-world requirements.

A typical in-game day may include:

1. Waking at home and checking plans or messages.
2. Going to work, school, or another ordinary obligation.
3. Exploring, completing errands, and meeting residents.
4. Choosing whom to spend limited time with.
5. Investigating anomalies toward evening.
6. Entering altered spaces or fighting at night.
7. Returning to a safe place, recording the day, and advancing time.

The main quest should allow breathing room for walking, meals, work, hobbies, errands, and optional relationships. These activities must create attachment or affect later content rather than serve only as decorative meters.

## 3. World structure

Build a dense open world of roughly four to six recognizable districts rather than pursuing maximum geographic size. Repeated visits should reveal changes caused by time, weather, relationships, quests, and memory instability.

The same location can have several states:

- An ordinary daytime state.
- An uneasy evening transition.
- A supernatural nighttime or deep-night state.
- A partially forgotten or rewritten state.
- An ending-specific restored, lost, or reconciled state.

Useful locations include homes, small shops, schools or workplaces, hospitals, transit stations, parks, rooftops, redevelopment zones, and a local festival area. Supernatural dungeons should usually grow from these familiar places instead of existing as disconnected fantasy realms.

### Time and atmosphere

- Morning emphasizes routine and preparation.
- Day appears safest but contains subtle inconsistencies.
- Evening weakens the boundary between layers.
- Night exposes hidden spaces and entities.
- Deep night produces the strongest mismatch between memory and reality.

A seasonal movement from early summer through rain and late summer into early autumn supports the tonal arc: sunlight and lively color gradually give way to humidity, cloud, longer shadows, and quieter streets.

## 4. Surface and hidden layer

The city's happiness is genuine, but it is sustained by a compromised system. Citizens have long surrendered unbearable grief, guilt, anger, and traumatic memories so they can resume daily life. Discarded memories accumulate beneath the city and produce entities and distorted spaces.

Erasure propagates through connections:

1. A painful event is removed.
2. Places associated with that event lose meaning or disappear.
3. Relationships formed there weaken or vanish.
4. A person whose identity is deeply connected to those memories can be erased from the entire city.

The organization operating this process, provisionally called the **Peace Management Bureau**, must have credible evidence that it reduced suffering, violence, or despair. It is an antagonist in methods and consequences, not a purely evil faction.

### Preferred signs of darkness

- A receipt names a shop that nobody remembers.
- A familiar seat remains unused after its owner is forgotten.
- CCTV skips a person but preserves reactions around their empty space.
- An NPC repeats a conversation without noticing.
- A group photo loses one participant after a quest.
- A shadow points against the light.
- A cheerful piece of music contains one subtly wrong note.

Use explicit horror sparingly so these details remain effective.

## 5. Protagonist and central relationship

The protagonist begins as an ordinary resident with work, school, financial, family, or social concerns. They initially regard daily repetition as unremarkable and have no heroic mission.

A close person, provisionally named **Yoon**, disappears. Everyone else, including records and family, acts as if Yoon never existed. The protagonist retains one photograph and a message:

> Even if I am forgotten, do not doubt what you saw.

The initial goal is intimate: prove that Yoon existed and find them. It later expands into protecting the city's ability to remember its people and share difficult memories.

Yoon ultimately serves as a memory anchor inside the erasure system, preventing a wider collapse. Yoon also participated in erasing part of the protagonist's past to protect them, creating a personal conflict that should allow forgiveness, honest anger, reconciliation, or separation without assigning a simplistic correct answer.

## 6. Main quest baseline

The working route is titled **The Empty Place in Memory**. Quest names and proper nouns are editable; the emotional progression should remain stable unless the user revises it.

### Prologue: The Same Day as Always

- Introduce home, routine, Yoon, neighbors, and ordinary controls through small errands.
- Record which people and places receive the player's attention.
- End with Yoon failing to arrive for a promise and being absent from every other memory and record.

### Act I: The Missing Place

- Search deleted contacts, old receipts, altered CCTV, and the background of the surviving photograph.
- Let repeated transit, nonexistent alleys, and forgotten seats make normal life increasingly unreliable.
- Enter the first hidden-space dungeon and recover a fragment showing that Yoon investigated the city.
- Conclude that Yoon was removed from collective memory rather than merely kidnapped.

### Act II: Those Who Remember

- Meet a small authored cast who remember other erased people: possible roles include a delivery worker, nurse, and student photographer.
- Reveal that shared time and strong relationships resist erasure.
- Explore each companion's ordinary life and unresolved pain.
- Follow a midnight train containing discarded memories and discover the Bureau's mark.

### Act III: The Price of Peace

- Enter or investigate the Bureau and learn the real benefits of memory treatment.
- Discover the propagation from erased pain to erased places, relationships, and people.
- Stage a bright city festival that gradually loses participants and records.
- Reveal that the festival infrastructure activates a large-scale treatment and that Yoon is the system's stabilizing anchor.

### Act IV: What Does Not Return

- Return fragments of citizens' grief and show social consequences that combat alone cannot solve.
- Revisit early locations; promises kept, time shared, and people ignored now alter their survival.
- Enter Yoon's memories and recover the protagonist's erased past.
- Give the player one final ordinary day to eat, walk, talk, organize belongings, and keep selected promises.

### Act V: The Center of Memory

- Merge the city's hidden spaces and travel to the system's core using accumulated relationships and memories.
- Confront **Peace**, a manifestation of the collective wish to live without pain, rather than a conventional evil final villain.
- Resolve the future of memory, Yoon, and the city through the player's established relationships and final choice.

### Ending baseline

- **Return the truth:** destroy erasure, restore memories and pain, and begin a difficult public recovery.
- **Preserve peace:** keep daily stability at the cost of the erased; the protagonist alone carries the truth.
- **Remember together:** when supported by sufficient relationships and fulfilled promises, replace erasure with shared remembrance. Some losses remain and recovery is imperfect.
- Relationship-specific epilogues should supplement these outcomes.

The epilogue returns to the protagonist's original daily route. Similar actions and scenery now carry different meaning. A suitable closing sentiment is: “Nothing special happened today. So I decided to remember this day for a long time.”

## 7. Systems that express the theme

### Memory record

- Preserve photographs, letters, objects, conversations, and the protagonist's own notes.
- Allow the world, map, or quest log to rewrite ordinary records while deliberately preserved evidence resists erasure.
- Use memories as clues and, where appropriate, as exploration or combat capabilities.

### Relationships

- Model trust, dependence, shared memory, promises, and unresolved conflict instead of reducing every bond to one affection number.
- Let time spent and promises kept affect companion behavior, safe spaces, world persistence, and endings.
- Give major NPCs personal routines and responsibilities. They should not wait indefinitely for the player.

### Everyday life

- Use meals, work, shopping, hobbies, walks, and appointments to establish relationships and place identity.
- Connect apparently small side quests to later changes: a rescued animal remains in the neighborhood; a helped shop becomes a meeting place; an ignored resident may leave only an empty home.
- Avoid turning ordinary life into repetitive survival-stat maintenance unless a mechanic reinforces character or theme.

### Choice and consequence

- Avoid a global good/evil meter.
- Track specific history and reveal consequences over time rather than immediately labeling choices.
- Do not require every activity in one playthrough. Limited time helps each player's city and attachments differ.

### Companions

- Use approximately three to five major companions if the scope supports it.
- Companions use AI in combat and retain independent schedules outside it.
- Their involvement, absence, memory loss, or departure follows story and relationship state.

### Saving

- Support practical autosaves around meaningful transitions and choices.
- A journal or photo album may present quest and memory state diegetically.
- Do not impose frustrating save restrictions solely to make choices feel important.

## 8. Presentation and technical baseline

The intended presentation is a depth-aware 2.5D world viewed from a quarter angle. The implementation may combine 3D terrain/buildings with sprite or mesh characters and effects. Maintain clear silhouettes, navigation readability, and correct occlusion.

Default coordinate convention:

- `X-Z`: world ground plane.
- `Y`: height.
- Screen and UI coordinates remain separate from world coordinates.
- Navigation data may use a grid or graph but must remain separate from authored transforms.

Foundation systems expected as development grows:

- Transform and math layer for vectors, matrices, and projections.
- Quarter-view camera with explicit projection and resize behavior.
- Depth-tested mesh and/or sprite rendering.
- Mouse picking from screen to world.
- Time-based game loop with separated update and render phases.
- Entity ownership and lifetime management.
- NPC schedule and relationship state.
- Quest state with consequences that can alter world records.
- Save/load for time, relationships, memory state, world state, and player choices.
- World partitioning, streaming, or another measured approach suitable for a dense open world.

The current `SimpleGame` implementation only opens a 500x500 FreeGLUT window and draws a solid rectangle through GLEW/OpenGL shaders. Before using it as an RPG foundation, address shader failure signaling, explicit OpenGL context requirements, GPU resource cleanup, resize handling, stable asset paths, clean-clone libraries/DLLs, and frame-independent updates.

## 9. Scope guidance

The earlier planning estimate was approximately 20-30 hours for the main story and 40-50 hours with major side content, 15-25 important recurring NPCs, and four to six districts. Treat these as direction rather than a production commitment until team size, schedule, assets, and tools are known.

When prototyping, prefer a vertical slice containing:

- One compact district.
- One complete daily cycle.
- A few recurring residents with schedules.
- One ordinary quest whose consequence returns later.
- One transition into a hidden-space dungeon.
- One short relationship branch.
- One memory change visible in the environment, journal, and NPC dialogue.

This slice should prove the relationship between ordinary life, subtle darkness, exploration, and 2.5D presentation before expanding map size.

## 10. Change control

Future design and code changes should answer:

- How does this help the player value a person, place, routine, or memory?
- Does it fit a single-player experience?
- Does it preserve readable quarter-view 2.5D play?
- Does it make the open world denser or more reactive rather than merely larger?
- Does it connect ordinary life to exploration, combat, quests, or endings?
- If it alters a fixed pillar, did the user explicitly authorize that change?

Update this guide when the user confirms a new pillar, replaces working canon, or approves a major system contract.
