import { createTask, filterTasks, progress } from "./model.js";

const storageKey = "focus-list-tasks";
const taskList = document.querySelector("#task-list");
const taskTemplate = document.querySelector("#task-template");
const form = document.querySelector("#task-form");
const input = document.querySelector("#task-input");
const filters = document.querySelectorAll(".filter");
let currentFilter = "all";
let tasks = loadTasks();

function loadTasks() {
  try {
    return JSON.parse(localStorage.getItem(storageKey)) || [];
  } catch {
    return [];
  }
}

function saveTasks() {
  localStorage.setItem(storageKey, JSON.stringify(tasks));
}

function formatCreatedAt(value) {
  const date = new Date(value);
  const today = new Date();
  if (date.toDateString() === today.toDateString()) {
    return `Today, ${date.toLocaleTimeString([], { hour: "numeric", minute: "2-digit" })}`;
  }
  return date.toLocaleDateString([], { month: "short", day: "numeric" });
}

function render() {
  taskList.innerHTML = "";
  const visibleTasks = filterTasks(tasks, currentFilter);

  if (!visibleTasks.length) {
    const empty = document.createElement("div");
    empty.className = "empty-state";
    empty.innerHTML = `<span>✦</span><strong>${tasks.length ? "Nothing here right now" : "Your day is wide open"}</strong><p>${tasks.length ? "Try another view to see your tasks." : "Add your first task above and build some momentum."}</p>`;
    taskList.append(empty);
  }

  visibleTasks.forEach((task) => {
    const item = taskTemplate.content.firstElementChild.cloneNode(true);
    item.dataset.id = task.id;
    item.classList.toggle("completed", task.completed);
    item.querySelector("p").textContent = task.title;
    item.querySelector("small").textContent = formatCreatedAt(task.createdAt);
    item.querySelector(".check").setAttribute("aria-label", task.completed ? "Mark task active" : "Mark task complete");
    taskList.append(item);
  });

  const percent = progress(tasks);
  document.querySelector("#progress-percent").textContent = `${percent}%`;
  document.querySelector("#progress-ring").style.setProperty("--progress", `${percent * 3.6}deg`);
  document.querySelector("#progress-copy").textContent = !tasks.length ? "A fresh start" : percent === 100 ? "Beautifully done" : `${tasks.filter((task) => task.completed).length} of ${tasks.length} complete`;
  document.querySelector("#all-count").textContent = tasks.length;
  document.querySelector("#clear-completed").disabled = !tasks.some((task) => task.completed);
}

form.addEventListener("submit", (event) => {
  event.preventDefault();
  tasks.unshift(createTask(input.value));
  input.value = "";
  saveTasks();
  render();
});

taskList.addEventListener("click", (event) => {
  const item = event.target.closest(".task-item");
  if (!item) return;
  if (event.target.closest(".check")) tasks = tasks.map((task) => task.id === item.dataset.id ? { ...task, completed: !task.completed } : task);
  if (event.target.closest(".delete")) tasks = tasks.filter((task) => task.id !== item.dataset.id);
  saveTasks();
  render();
});

filters.forEach((button) => button.addEventListener("click", () => {
  currentFilter = button.dataset.filter;
  filters.forEach((filter) => filter.classList.toggle("active", filter === button));
  render();
}));

document.querySelector("#clear-completed").addEventListener("click", () => {
  tasks = tasks.filter((task) => !task.completed);
  saveTasks();
  render();
});

document.querySelector("#current-date").textContent = new Date().toLocaleDateString([], { weekday: "long", month: "long", day: "numeric" });
render();
