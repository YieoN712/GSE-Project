"use strict";

// Rendering checks without building or launching the game.
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");
class Element {
    constructor() { this.textContent = ""; this.value = ""; this.children = []; }
    append(...children) { this.children.push(...children); }
    replaceChildren(...children) { this.children = children; }
    addEventListener() {}
}
const elements = new Map();
const document = {
    getElementById(id) {
        if (!elements.has(id)) elements.set(id, new Element());
        return elements.get(id);
    },
    createElement() { return new Element(); }
};
document.getElementById("kindFilter").value = "all";
const context = vm.createContext({
    document,
    Option: function (text, value) { this.textContent = text; this.value = value; },
    AbortController,
    setTimeout() { return 1; }, clearTimeout() {},
    fetch: async () => ({ ok: true, json: async () => ({ ready: false }) })
});
const html = fs.readFileSync(path.join(__dirname, "../SimpleGame/monitor/index.html"), "utf8");
vm.runInContext(html.match(/<script>([\s\S]*?)<\/script>/)[1], context);
const npc = {
    id: "18446744073709551615", townId: "haeeon", name: "<script>주민</script>",
    state: "talking", present: true, active: true, visible: true, position: { x: 1.25, y: 0, z: -2 }
};
context.data = {
    schemaVersion: 1, sequence: 1, scene: "tutorial", foreground: false, paused: false,
    simulationSeconds: 2, player: {}, npcs: [npc], mobs: [],
    counts: { npcs: 1, presentNpcs: 1, mobs: 0, aliveMobs: 0 },
    towns: [{ id: "haeeon", name: "해온", scope: "loaded-scene", population: 1,
        atmosphere: null, rumors: null, tendencies: null }]
};
vm.runInContext("validate(data); render(data)", context);
assert.equal(elements.get("npcCount").textContent, "1 / 1");
assert.equal(elements.get("mobCount").textContent, "0 / 0");
assert.match(elements.get("session").textContent, /시뮬레이션 정지/);
const cells = elements.get("entities").children[0].children;
assert.equal(cells[1].textContent, npc.id);
assert.equal(cells[2].textContent, npc.name);
assert.equal(cells[3].textContent, "대화");
assert.equal(cells[5].textContent, "1.3 / 0.0 / -2.0");
assert.equal(elements.get("towns").children[0].children[2].children[3].textContent, "미연동");
assert.equal(vm.runInContext("describe([])", context), "없음");
context.data.towns[0].atmosphere = "밝은 일상 속 외곽 걱정";
context.data.towns[0].rumors = ["귀갓길에 낯선 발소리가 들린다."];
context.data.towns[0].tendencies = { neighborsMet: 2, outskirtsReports: 1 };
context.data.towns[0].reputation = { trust: 4, label: "낯익은 얼굴" };
vm.runInContext("render(data)", context);
const townFields = elements.get("towns").children[0].children[2].children;
assert.equal(townFields[3].textContent, "밝은 일상 속 외곽 걱정");
assert.match(townFields[7].textContent, /인사한 이웃 2명/);
assert.equal(townFields[9].textContent, "낯익은 얼굴 · 신뢰 4");
elements.get("search").value = "not-found";
vm.runInContext("renderEntities()", context);
assert.equal(elements.get("entities").children[0].children[0].colSpan, 7);
elements.get("search").value = "";
elements.get("townFilter").value = "haeeon";
context.data.towns.push({ id: "other", name: "다른 마을", population: 0, rumors: [] });
context.data.mobs.push({ ...npc, id: "2", townId: "other", state: "dead", health: 0, maximumHealth: 40 });
context.data.counts.mobs = 1;
vm.runInContext("render(data)", context);
assert.equal(elements.get("townCount").textContent, 2);
assert.equal(elements.get("townFilter").value, "haeeon");
assert.match(elements.get("shown").textContent, /^1개 표시/);
elements.get("townFilter").value = "";
elements.get("kindFilter").value = "mob";
vm.runInContext("renderEntities()", context);
assert.equal(elements.get("entities").children[0].children[3].textContent, "사망 / 재생성 대기");
assert.throws(() => vm.runInContext("validate({schemaVersion: 99})", context));
console.log("Monitor dashboard checks passed.");
