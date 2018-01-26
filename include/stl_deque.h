#ifndef GRTW_STL_DEQUE_H
#define GRTW_STL_DEQUE_H

#include"stl_iterator.h"
#include"stl_construct.h"
#include"stl_allocator.h"
#include"stl_algorithms.h"
#include"stl_uninitialized.h"

namespace grtw
{
	template<class T, class Reference, class Pointer, size_t Nodesize>
	struct deque_iterator
	{
		using iterator_category = random_access_iterator_tag;
		using value_type = T;
		using difference_type = ptrdiff_t;
		using reference = Reference;
		using pointer = Pointer;

		using Self = deque_iterator<T, Reference, Pointer, Nodesize>;
		using iterator = deque_iterator<T, T&, T*, Nodesize>;
		using const_iterator = deque_iterator<T, const T&, const T*, Nodesize>;

		T* first;
		T* last;
		T* current;
		T** node;
		deque_iterator() : first(nullptr), last(nullptr), current(nullptr), node(nullptr) {}
		deque_iterator(T* x, T** y) : first(*y), last(*y + Nodesize), current(x), node(y) {}
		deque_iterator(const iterator& x) : first(x.first), last(x.last), current(x.current), node(x.node) {}

		reference operator*() const { return *current; }
		pointer operator->() const { return current; }

		difference_type operator-(const Self& x) const { return difference_type(Nodesize) * (node - x.node - 1) + (current - first) + (x.last - x.current); }
		void set_node(T** new_node)
		{
			node = new_node;
			first = *new_node;
			last = first + difference_type(Nodesize);
		}

		Self& operator++()
		{
			++current;
			if(current == last)
			{
				set_node(node + 1);
				current = first;
			}
			return *this;
		}

		Self operator++(int)
		{
			Self tmp = *this;
			++*this;
			return tmp;
		}

		Self& operator--()
		{
			if(current == first)
			{
				set_node(node - 1);
				current = last;
			}
			--current;
			return *this;
		}

		Self operator--(int)
		{
			Self tmp = *this;
			--*this;
			return tmp;
		}

		Self& operator+=(difference_type n)
		{
			difference_type offset = n + current - first;
			if(offset >= 0 && offset < difference_type(Nodesize))
				current += n;
			else
			{
				difference_type node_offset = offset > 0 ? offset / difference_type(Nodesize) : -difference_type((-offset - 1) / Nodesize) - 1;
				set_node(node + node_offset);
				current = first + (offset - node_offset * difference_type(Nodesize));				
			}
			return *this;
		}

		Self operator+(difference_type n) const
		{
			Self tmp = *this;
			return tmp += n;
		}

		Self& operator-=(difference_type n)
		{
			return *this += -n;
		}

		Self operator-(difference_type n) const
		{
			Self tmp = *this;
			return tmp -= n;
		}

		reference operator[](difference_type n)
		{
			return *(*this + n);
		}

		bool operator==(const Self& other) const { return current == other.current; }
		bool operator!=(const Self& other) const { return !(*this == other); }
		bool operator<(const Self& other) const { return (node == other.node) ? current < other.current : node < other.node; }
		bool operator>(const Self& other) const { return other < *this; }
		bool operator<=(const Self& other) const { return !(*this > other); }
		bool operator>=(const Self& other) const { return !(*this < other); }
	};

	template<class T, class Reference, class Pointer, size_t BufferSize>
	inline deque_iterator<T, Reference, Pointer, BufferSize> operator+(ptrdiff_t n, const deque_iterator<T>& it)
	{
		return it + n;
	}

	template<class T, class Node_Alloc = allocator<T>, class Map_Alloc = allocator<T*>>
	class deque
	{
	public:
		using value_type = T;
		using pointer = value_type*;
		using const_pointer = const value_type*;
		using reference = value_type&;
		using const_reference = const value_type&;
		using difference_type = ptrdiff_t;
		using size_type = size_t;

		using iterator = deque_iterator<T, reference, pointer, BufferSize()>;
		using const_iterator = deque_iterator<T, const_reference, const_pointer, BufferSize()>;
		using reverse_iterator = reverse_iterator<iterator>;
		using const_reverse_iterator = reverse_iterator<const_iterator>;

	private:
		T** node_map;
		size_t map_size;
		iterator start;
		iterator finish;

	private:
		static size_t BufferSize() { return (sizeof(T) < 512) ? (512/(sizeof(T))) : size_t(1); }

		void initialize_map(size_type);
		void create_nodes(T**, T**);
		void destroy_nodes(T**, T**);
		void fill_initialize(const value_type&);
		void check_map_and_realloc_map_if_not_enough_nodes_at_back(size_tye);
		void check_map_and_realloc_map_if_not_enough_nodes_at_front(size_type);
		void reallocate_map(size_type, bool);
		iterator insert_aux(iterator, const value_type&);
		iterator insert_aux(iterator, size_type, const value_type&);
		void insert_aux(iterator, const value_type*, const value_type*);
		void insert_aux(iterator, const_iterator, const_iterator);

	public:
		deque() : node_map(nullptr), map_size(0), start(), finish() { initialize_map(0); }
		explicit deque(size_type n) : node_map(nullptr), map_size(0), start(), finish()
		{
			initialize_map(n);
			fill_initialize(value_type());
		}

		deque(size_type n, const value_type& v) : node_map(nullptr), map_size(0), start(), finish()
		{
			initialize_map(n);
			fill_initialize(v);
		}

		deque(const value_type* first, const value_type* last) : node_map(nullptr), map_size(0), start(), finish()
		{
			initialize_map(last - first);
			uninitialized_copy(first, last, start);
		}

		deque(const_iterator first, const_iterator last) : node_map(nullptr), map_size(0), start(), finish()
		{
			initialize_map(last - first);
			uninitialized_copy(first, last, start);
		}

		deque(const deque& other) : node_map(nullptr), map_size(0), start(), finish()
		{
			initialize_map(other.size());
			uninitialized_copy(other.begin(), other.end(), start);
		}

		deque& operator=(const deque& other)
		{
			if(&other != this)
			{
				if(size() >= other.size())
					erase(copy(other.begin(), other.end(), start), finish);
				else
				{
					iterator mid = other.begin + difference_type(size());
					copy(other.begin(), mid);
					insert(finish, mid, other.end());
				}
			}
		}

		~deque()
		{
			destroy(start, finish);
			destroy_nodes(start.node, finish.node + 1);
			Map_Alloc::deallocate(node_map, map_size);
		}

		iterator begin() { return start; }
		iterator end() { return finish; }
		const_iterator begin() const { return start; }
		const_iterator end() const { return finish; }
		reverse_iterator rbegin() { return reverse_iterator(finish); }
		reverse_iterator rend() { return reverse_iterator(start); }
		const_reverse_iterator rbegin() const { return const_reverse_iterator(finish); }
		const_reverse_iterator rend() const { return const_reverse_iterator(start); }

		reference operator[](size_type n) { return start[difference_type(n)]; }
		const_reference operator[](size_type n) const { return start[difference_type(n)]; }

		reference front() { return *start; }
		const_reference front() const { return *start; }
		reference back()
		{
			iterator tmp = finish;
			return *(--tmp);
		}
		const_reference back() const
		{
			const_iterator tmp = finish;
			return *(--tmp);
		}

		size_type size() const { return finish - start; }
		bool empty() const { return finish == start; }

		void clear() { erase(start, finish); }

		void pop_back();
		void pop_front();

		void push_back(const value_type&);
		void push_front(const value_type&);
		iterator insert(iterator, const value_type&);
		iterator insert(iterator, size_type, const value_type&);
		void insert(iterator, const value_type*, const value_type*);
		void insert(iterator, const_iterator, const_iterator);
		void resize(size_type);
		iterator erase(iterator);
		iterator erase(iterator, iterator);
	};

	template<class T, class Node_Alloc, class Map_Alloc>
	void deque<T, Node_Alloc, Map_Alloc>::initialize_map(size_t nelements)
	{
		size_t nnodes = nelements/BufferSize() + 1;
		map_size = max(8, nnodes + 2);
		node_map = Map_Alloc::allocate(map_size);
		T** nstart = node_map + (map_size - nnodes)/2;
		T** nfinish = nstart + nnodes;
		create_nodes(nstart, nfinish);
		start.set_node(nstart);
		finish.set_node(nfinish - 1);
		start.current = start.first;
		finish.current = finish.first + nelements % BufferSize();
	}

	template<class T, class Node_Alloc, class Map_Alloc>
	void deque<T, Node_Alloc, Map_Alloc>::create_nodes(T** start, T** finish)
	{
		for(T** curr = start; curr < finish; ++curr)
			*curr = Node_Alloc::allocate(BufferSize());
	}

	template<class T, class Node_Alloc, class Map_Alloc>
	void deque<T, Node_Alloc, Map_Alloc>::destroy_nodes(T** start, T** finish)
	{
		for(T** curr = start; curr < finish; ++curr)
			Node_Alloc::deallocate(*curr, BufferSize());
	}

	template<class T, class Node_Alloc, class Map_Alloc>
	void deque<T, Node_Alloc, Map_Alloc>::fill_initialize(const T& v)
	{
		for(T** curr = start.node;; curr < finish.node; ++curr)
			uninitialized_fill(*curr, *curr + BufferSize(), v);
		uninitialized_fill(finish.first, finish.current, v);
	}

	template<class T, class Node_Alloc, class Map_Alloc>
	void deque<T, Node_Alloc, Map_Alloc>::reallocate_map(size_t nodes_to_add, bool add_at_front)
	{
		size_t old_node_nums = finish.node - start.node + 1;
		size_t new_node_nums = old_node_nums + nodes_to_add;
		T** new_start;
		if(map_size > 2 * new_node_nums)
		{
			new_start = node_map + (map_size - new_node_nums)/2 + (add_at_front ? nodes_to_add : 0);
			if(new_start < start)
				copy(start.node, finish.node + 1, new_start.node);
			else
				copy_backward(start.node, finish.node + 1, new_start + old_node_nums);
		}
		else
		{
			size_t new_map_size = map_size + max(nodes_to_add, map_size) + 2;
			T** new_map = Map_Alloc::allocate(new_map_size);
			new_start = new_map + (new_map_size - new_node_nums)/2 + (add_at_front ? nodes_to_add : 0);
			copy(start.node, finish.node + 1, new_start);
			Map_Alloc::deallocate(node_map, map_size);
			node_map = new_map;
			map_size = new_map_size;
		}
		start.set_node(new_start);
		finish.set_node(new_start + old_node_nums - 1);
	}

	template<class T, class Node_Alloc, class Map_Alloc>
	void deque<T, Node_Alloc, Map_Alloc>::check_map_and_realloc_map_if_not_enough_nodes_at_back(size_type nodes_to_add)
	{
		if(map_size - (finish.node - node_map) < nodes_to_add + 1)
			reallocate_map(nodes_to_add, false);
	}

	template<class T, class Node_Alloc, class Map_Alloc>
	void deque<T, Node_Alloc, Map_Alloc>::check_map_and_realloc_map_if_not_enough_nodes_at_front(size_type nodes_to_add)
	{
		if(nodes_to_add > size_type(start.node - node_map))
			reallocate_map(nodes_to_add, true);
	}

	template<class T, class Node_Alloc, class Map_Alloc>
	void deque<T, Node_Alloc, Map_Alloc>::push_back(const value_type& v)
	{
		if(finish.current != finish.last - 1)
			construct(finish.current, v);
		else
		{
			check_map_and_realloc_map_if_not_enough_nodes_at_back(1);
			*(finish.node + 1) = Node_Alloc::allocate(BufferSize());
			construct(finish.current, v);
		}
		++finish;
	}

	template<class T, class Node_Alloc, class Map_Alloc>
	void deque<T, Node_Alloc, Map_Alloc>::push_front(const value_type& v)
	{
		if(start.current != start.first)
			construct(start.current - 1, v);
		else
		{
			check_map_and_realloc_map_if_not_enough_nodes_at_front(1);
			*(start.node - 1) = Node_Alloc::allocate(BufferSize());
			construct(*(start.node - 1) + BufferSize() - 1, v);
		}
		--start;
	}

	template<class T, class Node_Alloc, class Map_Alloc>
	typename deque<T, Node_Alloc, Map_Alloc>::iterator
	deque<T, Node_Alloc, Map_Alloc>::insert(iterator it, const T& v)
	{
		if(it == finish)
		{
			push_back(v);
			return finish - 1;
		}
		else if(it == start)
		{
			push_front(v);
			return start;
		}
		else
		{
			return insert_aux(it, v);
		}
	}

	template<class T, class Node_Alloc, class Map_Alloc>
	typename deque<T, Node_Alloc, Map_Alloc>::iterator
	deque<T, Node_Alloc, Map_Alloc>::insert_aux(iterator it, const T& v)
	{
		difference_type index = it - start;
		value_type v_copy = v;
		if(size_type(index) < size() / 2)
		{
			push_front(front());
			iterator front1 = start + 1;
			iterator front2 = front1 + 1;
			iterator pos = start + index + 1;
			copy(front2, pos, front1);
			*pos = v_copy;
			return pos;
		}
		else
		{
			 push_back(back());
			 iterator back1 = finish - 1;
			 iterator back2 = back1 - 1;
			 iterator pos = start + index;
			 copy_backward(index, back2, back1);
			 *pos = v_copy;
			 return pos;
		}
	}

	template<class T, class Node_Alloc, class Map_Alloc>
	typename deque<T, Node_Alloc, Map_Alloc>::iterator
	deque<T, Node_Alloc, Map_Alloc>::insert(iterator it, size_type n, const T& v)
	{
		if(it == finish)
		{
			size_type vacancies = finish.last - finish.current - 1;
			if(vacancies < n)
			{
				size_type nodes_to_add = (n - vacancies + BufferSize() - 1) / BufferSize();
				check_map_and_realloc_map_if_not_enough_nodes_at_back(nodes_to_add);
				for(size_type i = 1; i <= nodes_to_add; i++)
					*(finish.node + i) = Node_Alloc::allocate(BufferSize());
			}
			iterator new_finish = finish + difference_type(n);
			uninitialized_fill(finish, new_finish, v);
			finish = new_finish;
			return finish - difference_type(n);
		}
		else if(it == start)
		{
			size_type vacancies = start.current - start.first;
			if(vacancies < n)
			{
				size_type nodes_to_add = (n - vacancies + BufferSize() - 1) / BufferSize();
				check_map_and_realloc_map_if_not_enough_nodes_at_front(nodes_to_add);
				for(size_type i = 1; i <= nodes_to_add; i++)
					*(start.node - i) = Node_Alloc::allocate(BufferSize());
			}
			iterator new_start = start - difference_type(n);
			uninitialized_fill(new_start, start, v);
			start = new_start;
			return start;
		}
		else
		{
			return insert_aux(it, n, v);
		}
	}

	template<class T, class Node_Alloc, class Map_Alloc>
	typename deque<T, Node_Alloc, Map_Alloc>::iterator
	deque<T, Node_Alloc, Map_Alloc>::insert_aux(iterator it, size_type n, const T& v)
	{
		const difference_type elem_before = it - start;
		value_type v_copy = v;
		if(elem_before < difference_type(size() / 2))
		{
			size_type vacancies = start.current - start.first;
			if(vacancies < n)
			{
				size_type nodes_to_add = (n - vacancies + BufferSize() - 1) / BufferSize();
				check_map_and_realloc_map_if_not_enough_nodes_at_front(nodes_to_add);
				for(size_type i = 1; i <= nodes_to_add; ++i)
					*(start.node - i) = Node_Alloc::allocate(BufferSize());
			}
			iterator new_start = start - n;
			iterator old_start = start;
			iterator pos = start + elem_before;
			if(elem_before >= n)
			{
				iterator start_n = start + n;
				uninitialized_copy(start, start_n, new_start);
				copy(start_n, pos, start);
				start = new_start;
				fill(pos - difference_type(n), pos, v_copy);
			}
			else
			{
				uninitialized_copy(start, pos, new_start);
				uninitialized_fill(new_start + elem_before, old_start, v_copy);
				fill(old_start, pos, v_copy);
				start = new_start;
			}
			return pos;
		}
		else
		{
			size_type vacancies = finish.last - finish.current - 1;
			if(vacancies < n)
			{
				size_type nodes_to_add = (n - vacancies + BufferSize() - 1) / BufferSize();
				check_map_and_realloc_map_if_not_enough_nodes_at_back(nodes_to_add);
				for(int i = 1; i <= nodes_to_add; ++i)
					*(finish.node + i) = Node_Alloc::allocate(BufferSize())
			}
			iterator new_finish = finish + n;
			iterator old_finish = finish;
			const difference_type elem_after = difference_type(size()) - elem_before;
			iterator pos = finish - elem_after;
			if(elem_after > difference_type(n))
			{
				iterator finish_n = old_finish - n;
				uninitialized_copy(finish_n, old_finish, old_finish);
				copy_backward(pos, finish_n, old_finish);
				finish = new_finish;
				fill(pos, pos + difference_type(n), v_copy);
			}
			else
			{
				uninitialized_copy(pos, old_finish, pos + difference_type(n));
				uninitialized_fill(old_finish, pos + difference_type(n), v_copy);
				fill(pos, old_finish, v_copy);
				finish = new_finish;
			}
			return pos;
		}
	}

	template<class T, class Node_Alloc, class Map_Alloc>
	void deque<T, Node_Alloc, Map_Alloc>::insert(iterator it, const value_type* vfirst, const value_type* vlast)
	{
		if(it == finish)
		{

		}
		else if(it == start)
		{

		}
		else
			insert_aux(it, vfirst, vlast);
	}

	template<class T, class Node_Alloc, class Map_Alloc>
	void deque<T, Node_Alloc, Map_Alloc>::insert_aux(iterator it, const value_type* vfirst, const value_type* vlast)
	{

	}

	template<class T, class Node_Alloc, class Map_Alloc>
	void deque<T, Node_Alloc, Map_Alloc>::insert(iterator it, const_iterator vfirst, const_iterator vlast)
	{
		if(it == finish)
		{

		}
		else if(it == start)
		{

		}
		else
			insert_aux(it, vfirst, vlast);
	}

	template<class T, class Node_Alloc, class Map_Alloc>
	void deque<T, Node_Alloc, Map_Alloc>::insert_aux(iterator it, const_iterator vfirst, const_iterator vlast)
	{

	}

	template<class T, class Node_Alloc, class Map_Alloc>
	void deque<T, Node_Alloc, Map_Alloc>::pop_back()
	{

	}

	template<class T, class Node_Alloc, class Map_Alloc>
	void deque<T, Node_Alloc, Map_Alloc>::pop_front()
	{

	}

	template<class T, class Node_Alloc, class Map_Alloc>
	void deque<T, Node_Alloc, Map_Alloc>::erase(iterator it)
	{
		if(it == finish - 1)
			pop_back();
		else if(it == start)
			pop_front();
		else
		{

		}
	}

	template<class T, class Node_Alloc, class Map_Alloc>
	void deque<T, Node_Alloc, Map_Alloc>::erase(iterator vfirst, iterator vlast)
	{
		
	}
}

#endif