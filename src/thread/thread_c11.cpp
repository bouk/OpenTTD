/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

#include "../stdafx.h"
#include "../debug.h"
#include "thread.h"
#include <switch.h>
#include <threads.h>

/** @file thread_c11.cpp C11 implementation of Threads. */

class ThreadObject_c11 : public ThreadObject {
private:
	thrd_t thread;    ///< System thread identifier.
	OTTDThreadFunc proc; ///< External thread procedure.
	void *param;         ///< Parameter for the external thread procedure.
	const char* name;
	bool self_destruct;  ///< Free ourselves when done?

public:
	/**
	 * Create a thrd and start it, calling proc(param).
	 */
	ThreadObject_c11(OTTDThreadFunc proc, void *param, bool self_destruct, const char* name) :
		thread(0),
		proc(proc),
		param(param),
		name(name),
		self_destruct(self_destruct)
	{
		DEBUG(console, 8, "creating thread %s", name);
		thrd_create(&this->thread, (thrd_start_t)&stThreadProc, this);
	}

	/* virtual */ bool Exit()
	{
		assert(thrd_current() == this->thread);
		/* For now we terminate by throwing an error, gives much cleaner cleanup */
		throw OTTDThreadExitSignal();
	}

	/* virtual */ void Join()
	{
		/* You cannot join yourself */
		assert(thrd_current() != this->thread);
		thrd_join(this->thread, NULL);
		this->thread = 0;
	}
private:
	/**
	 * On thread creation, this function is called, which calls the real startup
	 *  function. This to get back into the correct instance again.
	 */
	static int stThreadProc(void *thr)
	{
		ThreadObject_c11 *self = (ThreadObject_c11 *) thr;
		self->ThreadProc();
		return 0;
	}

	/**
	 * A new thread is created, and this function is called. Call the custom
	 *  function of the creator of the thread.
	 */
	void ThreadProc()
	{
		DEBUG(console, 8, "starting thread %s", name);
		/* Call the proc of the creator to continue this thread */
		try {
			this->proc(this->param);
		} catch (OTTDThreadExitSignal) {
		} catch (...) {
			NOT_REACHED();
		}
		DEBUG(console, 8, "finished thread %s", name);

		if (self_destruct) {
			// TODO(bouk): not implemented in libnx right now
			// thrd_detach(thrd_current());
			delete this;
		}
	}
};

/* static */ bool ThreadObject::New(OTTDThreadFunc proc, void *param, ThreadObject **thread, const char *name)
{
	ThreadObject *to = new ThreadObject_c11(proc, param, thread == NULL, name);
	if (thread != NULL) *thread = to;
	return true;
}

class ThreadMutex_c11 : public ThreadMutex {
private:
	mtx_t mutex;    ///< The actual mutex.
	cnd_t condition; ///< Data for conditional waiting.
	thrd_t owner;          ///< Owning thread of the mutex.
	uint recursive_count;     ///< Recursive lock count.

public:
	ThreadMutex_c11() : owner((thrd_t)1), recursive_count(0)
	{
		mtx_init(&this->mutex, mtx_plain);
		cnd_init(&this->condition);
	}

	/* virtual */ ~ThreadMutex_c11()
	{
		cnd_destroy(&this->condition);
		mtx_destroy(&this->mutex);
	}

	bool IsOwnedByCurrentThread() const
	{
		return this->owner == thrd_current();
	}

	/* virtual */ void BeginCritical(bool allow_recursive = false)
	{
		/* C11 mutex is not recursive by itself */
		if (this->IsOwnedByCurrentThread()) {
			if (!allow_recursive) NOT_REACHED();
		} else {
			int err = mtx_lock(&this->mutex);
			assert(err == thrd_success);
			assert(this->recursive_count == 0);
			this->owner = thrd_current();
		}
		this->recursive_count++;
	}

	/* virtual */ void EndCritical(bool allow_recursive = false)
	{
		assert(this->IsOwnedByCurrentThread());
		if (!allow_recursive && this->recursive_count != 1) NOT_REACHED();
		this->recursive_count--;
		if (this->recursive_count != 0) return;
		this->owner = (thrd_t)1;
		int err = mtx_unlock(&this->mutex);
		assert(err == thrd_success);
	}

	/* virtual */ void WaitForSignal()
	{
		uint old_recursive_count = this->recursive_count;
		this->recursive_count = 0;
		this->owner = (thrd_t)1;
		int err = cnd_wait(&this->condition, &this->mutex);
		assert(err == thrd_success);
		this->owner = thrd_current();
		this->recursive_count = old_recursive_count;
	}

	/* virtual */ void SendSignal()
	{
		int err = cnd_signal(&this->condition);
		assert(err == thrd_success);
	}
};

/* static */ ThreadMutex *ThreadMutex::New()
{
	return new ThreadMutex_c11();
}
