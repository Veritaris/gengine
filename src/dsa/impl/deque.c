//
// Created by Георгий Имешкенов on 23.06.2024.
//

#include "deque.h"

any push_front(struct deque_s *self, any element) {

}

any pop_front(struct deque_s *self) {
    return self->_storage->removeAt(self->_storage, 0);
}

any push_back(struct deque_s *self, any element) {
    return self->_storage->push(self->_storage, element);
}

any pop_back(struct deque_s *self) {

}


Deque_t *Deque(const size_t *init_capacity) {
    ArrayList_t *_storage = ArrayList(init_capacity);
    Deque_t *_deque;
    cmalloc_safe(_deque, sizeof(Deque_t))

    _deque->first_elem_index = 0;
    _deque->last_elem_index = 0;
    _deque->_storage = _storage;
    _deque->push_front = push_front;
    _deque->push_back = push_back;
    _deque->pop_front = pop_front;
    _deque->pop_back = pop_back;

    return _deque;
}
