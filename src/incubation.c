	/* Unstable Scroll Incubation */
	if (p_ptr->scroll_delay > 0)
	{
		p_ptr->scroll_delay--;
		/* Atmospheric messages */
		if (rand_int(100) < 50) /* 50% chance per turn */
		{
			switch (rand_int(3))
			{
				case 0: msg_print("You feel a cold sweat break out."); break;
				case 1: msg_print("The shadows around you seem to lengthen."); break;
				case 2: msg_print("A faint humming vibrates in your teeth."); break;
			}
		}

		if (p_ptr->scroll_delay == 0)
		{
			/* Trigger */
			trigger_unstable_effect(p_ptr->scroll_pending_effect, FALSE);
		}
	}
