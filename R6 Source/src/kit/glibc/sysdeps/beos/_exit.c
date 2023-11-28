/* I have no idea whether this is a good idea or not.  --drepper */
extern void _kexit_team_ (int status);

void
_exit (int status)
{
  _kexit_team_ (status);
}
