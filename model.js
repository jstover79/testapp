export function createTask(title, now = new Date()) {
  const cleanTitle = title.trim();
  if (!cleanTitle) throw new Error("A task needs a title");

  return {
    id: `${now.getTime()}-${Math.random().toString(36).slice(2, 8)}`,
    title: cleanTitle,
    completed: false,
    createdAt: now.toISOString(),
  };
}

export function filterTasks(tasks, filter) {
  if (filter === "active") return tasks.filter((task) => !task.completed);
  if (filter === "completed") return tasks.filter((task) => task.completed);
  return tasks;
}

export function progress(tasks) {
  if (!tasks.length) return 0;
  return Math.round((tasks.filter((task) => task.completed).length / tasks.length) * 100);
}
