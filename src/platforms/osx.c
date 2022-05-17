#include "core.h"
#include "platform.h"

Mutex *Mutex_Create(void) {
	Mutex *ptr = Memory_Alloc(1, sizeof(Mutex));
	cs_int32 ret;

	if((ret = pthread_mutex_init(&ptr->handle, NULL)) != 0) {
		_Error_Print(ret, true);
	}

	if((ret = pthread_cond_init(&ptr->cond, NULL)) != 0) {
		_Error_Print(ret, true);
	}

	ptr->owner = NULL;
	ptr->rec = 0;
	return ptr;
}

void Mutex_Free(Mutex *mtx) {
	pthread_cond_destroy(&mtx->cond);
	pthread_mutex_destroy(&mtx->handle);
	Memory_Free(mtx);
}

void Mutex_Lock(Mutex *mtx) {
	pthread_t th = pthread_self();
	cs_int32 ret;
	if((ret = pthread_mutex_lock(&mtx->handle)) != 0) {
		_Error_Print(ret, true);
	}
	if(mtx->owner == th)
		mtx->rec++;
	else {
		while(mtx->rec) {
			if((ret = pthread_cond_wait(&mtx->cond, &mtx->handle)) != 0) {
				_Error_Print(ret, true);
			}
		}
		mtx->owner = th;
		mtx->rec = 1;
	}
	if((ret = pthread_mutex_unlock(&mtx->handle)) != 0) {
		_Error_Print(ret, true);
	}
}

void Mutex_Unlock(Mutex *mtx) {
	cs_int32 ret;
	if((ret = pthread_mutex_lock(&mtx->handle)) != 0) {
		_Error_Print(ret, true);
	}
	if(--mtx->rec == 0) {
		mtx->owner = NULL;
		if((ret = pthread_cond_signal(&mtx->cond)) != 0) {
			_Error_Print(ret, true);
		}
	}
	if((ret = pthread_mutex_unlock(&mtx->handle)) != 0) {
		_Error_Print(ret, true);
	}
}

Semaphore *Semaphore_Create(cs_ulong initial, cs_ulong max) {

}

cs_bool Semaphore_TryWait(Semaphore *sem, cs_ulong time) {

}

void Semaphore_Wait(Semaphore *sem) {

}

void Semaphore_Post(Semaphore *sem) {

}

void Semaphore_Free(Semaphore *sem) {

}
