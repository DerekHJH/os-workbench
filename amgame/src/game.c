#include <game.h>

// Operating system is a C program!
_settings Settings = {.ship_x = 0, .ship_y = 0, .ship_side = 20, .ship_color = 0x880000, .ship_speed = 10};
_ship Ship;
void ship_init()
{
	Ship.x = Settings.ship_x; 
	Ship.y = Settings.ship_y; 
	Ship.Side = Settings.ship_side; 
	Ship.Color = Settings.ship_color; 
	Ship.Speed = Settings.ship_speed;
}
void Initialize()
{
	ship_init();
}
void check_keyboard()
{
  int Key = read_key();
  if((Key & 0x8000) == 0)return;
  switch(Key ^ 0x8000)
  {
    case _KEY_ESCAPE:
    {
      _halt(0);
      break;
    }
    case _KEY_UP:
    {
      Ship.y -= Settings.ship_speed;
      break;
    }
    case _KEY_DOWN:
    {
      Ship.y += Settings.ship_speed;
      break;
    }
    case _KEY_LEFT:
    {
      Ship.x -= Settings.ship_speed;
      break;
    }
    case _KEY_RIGHT:
    {
      Ship.x += Settings.ship_speed;
      break;
    }
    default:
    {
      break;
    }
  }
}
void check_events()
{
	check_keyboard();
}
void screen_update()
{
  draw_ship();
}
int main(const char *args) 
{
  _ioe_init();
  /*
  puts("mainargs = \"");
  puts(args); // make run mainargs=xxx
  puts("\"\n");

  splash();

  puts("Press any key to see its key code...\n");
  while (1) 
	{
    print_key();
  }
  */
	Initialize();
  while(1)
  {
		printf("%d\n", uptime());
    check_events();
    screen_update();
  }



  return 0;
}
