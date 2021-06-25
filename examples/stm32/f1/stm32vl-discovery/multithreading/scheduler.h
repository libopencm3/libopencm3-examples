/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2015 Jonas Norling <jonas.norling@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

void scheduler_init(void);
void scheduler_yield(void);

#define SCHEDULER_NUM_TASKS 2

/*
 * Constant information about the system's tasks.
 */
struct task_data {
	void (*entry_point)(void);
	void *stack;
	unsigned stack_size;
};

extern const struct task_data tasks[SCHEDULER_NUM_TASKS];

#endif /* _SCHEDULER_H_ */
