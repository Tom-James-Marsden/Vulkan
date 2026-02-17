#define TINYOBJLOADER_IMPLEMENTATION 

#include "Window.h"

int main() {
	
	Window wnd;

	wnd.Init(1200,800);



	while (!wnd.ShouldClose())
		wnd.Update();
	
	wnd.Destroy();



}