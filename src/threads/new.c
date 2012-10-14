void
add_to_list(struct list *list, struct list_elem *thread_elem)
{
  struct thread *insertThread = list_entry(thread_elem, struct thread, elem), *beforeThread;

  ASSERT (is_thread (insertThread));

  for ( e = list_begin (list); e != list_end (list); e = list_next (e))
    {
      beforeThread = list_entry(e, struct thread, elem);
      
      ASSERT (is_thread (beforeThread));0
      if ( insertThread->priority > beforeThread->priority )
	{
	  list_insert (thread_elem, e);
	  break;
	}
    }
}

// simply "reuse" list_push_back, but replace list_insert by list_insert_ordered

/* Inserts ELEM at the end of LIST, so that it becomes the
   back in LIST. */

list_less_func threadLess(const struct list_elem *a, const struct list_elem *b, void *aux)
{
  struct thread *threadA = list_entry(a, struct thread, elem);
  struct thread *threadB = list_entry(b, struct thread, elem);

  ASSERT (is_thread (threadA));
  ASSERT (is_thread (threadB));
  
  if (threadA->priority < threadB->priority)
    return true;
  else
    return false;
}

void
list_push_back (struct list *list, struct list_elem *elem)
{
  list_insert_ordered (list, elem, threadLess, NULL);
}

