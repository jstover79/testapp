import test from "node:test";
import assert from "node:assert/strict";
import { createTask, filterTasks, progress } from "../model.js";

test("createTask trims the title and creates an active task", () => {
  const now = new Date("2026-09-04T12:00:00.000Z");
  const task = createTask("  Ship the app  ", now);
  assert.equal(task.title, "Ship the app");
  assert.equal(task.completed, false);
  assert.equal(task.createdAt, now.toISOString());
});

test("createTask rejects an empty title", () => {
  assert.throws(() => createTask("   "), /needs a title/);
});

test("filterTasks returns the requested task state", () => {
  const tasks = [{ completed: false }, { completed: true }];
  assert.deepEqual(filterTasks(tasks, "active"), [tasks[0]]);
  assert.deepEqual(filterTasks(tasks, "completed"), [tasks[1]]);
  assert.deepEqual(filterTasks(tasks, "all"), tasks);
});

test("progress rounds completion to a whole percentage", () => {
  assert.equal(progress([]), 0);
  assert.equal(progress([{ completed: true }, { completed: false }, { completed: false }]), 33);
  assert.equal(progress([{ completed: true }]), 100);
});
