#ifndef GRAPHICS2_HPP_
#define GRAPHICS2_HPP_

#include <string>

struct SDL_Surface;
struct SDL_Window;
struct _TTF_Font;

using Uint8  = unsigned char;
using Uint32 = unsigned int;

namespace genv
{
/* Output device definitions */
class canvas;

struct font {
	std::string name;
	int size;
	bool antialias;

	font()
		: name(std::string()), size(16), antialias(true)
	{
	}

	font(std::string name_, int size_ = 16, bool antialias_ = true)
		: name(name_), size(size_), antialias(antialias_)
	{
		if (size < 16) size = 16;
	}
};

struct rect { 
	int x, y;
	int w, h;
};

struct point {
	int x, y;
};

struct stamp {
	canvas &c;
	int s_x, s_y, s_w, s_h;
	int d_x, d_y;

	stamp(canvas &src_c, int src_x, int src_y, int src_w, int src_h, int dest_x, int dest_y)
		: c(src_c), 
			s_x(src_x), s_y(src_y), s_w(src_w), s_h(src_h), 
			d_x(dest_x), d_y(dest_y)
	{
	}

	stamp(canvas &src_c, int dest_x, int dest_y)
		: c(src_c), 
			s_x(-1), s_y(-1), s_w(-1), s_h(-1), 
			d_x(dest_x), d_y(dest_y)
	{
	}
};

struct color {
	Uint8 r, g, b;
	color() : r(0xFF), g(0xFF), b(0xFF) { }
	color(Uint8 red, Uint8 green, Uint8 blue) : r(red), g(green), b(blue) { }
};

struct move {
	int diff_x, diff_y;
	move(int dx, int dy) : diff_x(dx), diff_y(dy) { }
};

struct move_to {
	int coord_x, coord_y;
	move_to(int x, int y) : coord_x(x), coord_y(y) { }
};

struct line {
	int diff_x, diff_y;
	line(int dx, int dy) : diff_x(dx), diff_y(dy) { }
};

struct line_to {
	int coord_x, coord_y;
	line_to(int x, int y) : coord_x(x), coord_y(y) { }
};

struct box {
	int w, h;
	box(int width, int height) : w(width), h(height) { }
};

struct box_to {
	int bottomright_x, bottomright_y;
	box_to(int x, int y) : bottomright_x(x), bottomright_y(y) { }
};

struct text {
	std::string str;
	text(std::string s) : str(s) { }
	text(char c) : str(1, c) { }
};

class canvas {
private:
	bool in_bounds(int x, int y);
	Uint32& pixel(int x, int y);

	void draw_line(int x, int y, bool relative = false);
	void draw_text(const char* text, size_t text_length);
	bool load_font(font f);

protected:
	int draw_x, draw_y;
	int draw_color;
	color color_info;

	SDL_Surface* buf;
	bool transparent;

	_TTF_Font *loaded_font;
	font font_info;

public:
	canvas();
	canvas(int w, int h);
	canvas(const canvas &c);
    
	virtual ~canvas();

	bool open(int width, int height);
	bool save(const std::string &filename) const;
	void dot();

	int width() const;
	int height() const;

	bool has_font() const { return loaded_font != NULL; }
	int cascent() const;
	int cdescent() const;
	int twidth(const std::string &s) const;

	void set_transparent(bool transp) { transparent = transp; }
	void set_antialias(bool aa) { font_info.antialias = aa; }

	canvas& operator=(const canvas &c);
	canvas& operator<<(font f);
	canvas& operator<<(stamp s);
	canvas& operator<<(color c);
	canvas& operator<<(const move &m);
	canvas& operator<<(const move_to &mt);
	canvas& operator<<(const line &l);
	canvas& operator<<(const line_to &lt);
	canvas& operator<<(const box &b);
	canvas& operator<<(const box_to &bt);
	canvas& operator<<(const text &t);
	canvas& operator<<(const std::string &t);
	canvas& operator<<(const char* t);
};

// Generic operator for applying global manipulators
template<typename Func>
inline canvas& operator<<(canvas &c, Func f) {
	f(c);
	return c;
}

// Class of output device (singleton)
class groutput : public canvas {
protected:
	SDL_Window *window;
	groutput();

public:
	static groutput &instance();
	virtual ~groutput() override;

	void refresh();
	bool open(
		int width, 
		int height, 
		std::string title = "SDL App", 
		bool fullscreen = false);

	void show_mouse(bool shown);
	void move_mouse(int x, int y);
};

// Global accessor for the output device instance
extern groutput &gout;

// Global manipulators for frequently used operations
inline void refresh(canvas &) { gout.refresh(); }
inline void dot(canvas &c) { c.dot(); }

/*  Input device definitions  */
enum keycode_t {
	key_tab = '\t', 
	key_backspace = '\b', 
	key_enter = '\r',
	key_escape = '\033', 
	key_space = ' ',

	key_up = 0x10000, key_down, key_right, key_left,
	key_insert, key_delete, key_home, key_end, key_pgup, key_pgdn,
	key_lshift, key_rshift, key_lctrl, key_rctrl, key_lalt, key_ralt,
	key_lwin, key_rwin, key_menu, key_numl, key_capsl, key_scrl,

	key_f0 = 0x20000, key_f1, key_f2, key_f3, key_f4, key_f5, key_f6, key_f7,
	key_f8, key_f9, key_f10, key_f11, key_f12, key_f13, key_f14, key_f15
};

enum button_t {
	btn_left = 1, 
	btn_middle, 
	btn_right, 
	btn_wheelup, 
	btn_wheeldown
};

enum event_type {
	ev_key = 1, 
	ev_mouse, 
	ev_timer
};

struct event
{
	int keycode;
	int pos_x, pos_y;
	int button;
	int time;
	int type;
};

// Class of input device (singleton)
class grinput
{
private:
	grinput() : quit(false) { }
	bool quit;

public:
	static grinput& instance();

	grinput& wait_event(event&);
	void timer(int interval);

	operator const void*() const
	{ if (quit) return 0; else return this; }
};

// Global accessor for the input device instance
extern grinput& gin;

inline grinput& operator>>(grinput& inp, event& ev) { 
	return inp.wait_event(ev); 
}

}

#endif