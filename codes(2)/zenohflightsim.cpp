#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Userenv.lib")
#pragma comment(lib, "Bcrypt.lib")
#pragma comment(lib, "oe_base.lib")
#pragma comment(lib, "oe_graphics.lib")
#pragma comment(lib, "oe_simulation.lib")

#include "openeaagles/base/Object.hpp"
#include "zenoh.h"
using namespace std;
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

z_owned_session_t session;
double posX = -2.0;
double posY = -2.0;
double speedKts = 250.0;
double accelKts2 = 2.5;
double headingDeg = 45.0;
int frame = 0;

void updatePhysicsAndTransmit(double dt) {
	speedKts += accelKts2 * dt;
	double distNM = (speedKts / 3600.0) * dt;
	double headingRad = headingDeg * (M_PI / 180.0);

	posX += distNM * sin(headingRad);
	posY += distNM * cos(headingRad);
	frame++;

	if (posX > 3.0 || posY > 3.0) {
		posX = -2.5;
		posY = -2.5;
	}

	stringstream ss;
	ss << fixed << setprecision(4);
	ss << "{"
		<< "\"frame\":" << frame << ","
		<< "\"dt\":" << dt << ","
		<< "\"x_nm\":" << posX << ","
		<< "\"y_nm\":" << posY
		<< "}";

	string payloadStr = ss.str();

	z_owned_bytes_t payloadBytes;
	z_bytes_from_static_str(&payloadBytes, payloadStr.c_str());

	z_view_keyexpr_t keyExpression;
	z_view_keyexpr_from_str(&keyExpression, "simulation/aircraft/position");

	z_put(z_loan(session), z_loan(keyExpression), z_move(payloadBytes), NULL);
	cout << "[TRANSMITTING POS] -> " << payloadStr << endl;
}

void drawRadarDisplay() {
	glClear(GL_COLOR_BUFFER_BIT);
	glLoadIdentity();
	glColor3f(0.0f, 0.3f, 0.0f);
	glBegin(GL_LINES);
	for (float i = -3.0f; i <= 3.0f; i += 1.0f) {
		glVertex2f(i, -3.0f); glVertex2f(i, 3.0f);
		glVertex2f(-3.0f, i); glVertex2f(3.0f, i);
	}
	glEnd();
	glColor3f(0.0f, 0.6f, 0.0f);
	glBegin(GL_LINES);
	glVertex2f(-3.0f, 0.0f); glVertex2f(3.0f, 0.0f);
	glVertex2f(0.0f, -3.0f); glVertex2f(0.0f, 3.0f);
	glEnd();
	glPushMatrix();
	glTranslatef((GLfloat)posX, (GLfloat)posY, 0.0f);
	glColor3f(1.0f, 0.0f, 0.0f);

	glBegin(GL_POLYGON);
	glVertex2f(0.0f, 0.15f);
	glVertex2f(-0.1f, -0.1f);
	glVertex2f(0.0f, -0.05f);
	glVertex2f(0.1f, -0.1f);
	glEnd();
	glPopMatrix();
}
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

int main(int argc, char* argv[]) {
	cout << "======================================================" << endl;
	cout << "   TRANSMITTER: OPENEAGLES SIMULATOR & ZENOH          " << endl;
	cout << "======================================================" << endl;

	z_owned_config_t config;
	z_config_default(&config);
	if (z_open(&session, z_move(config), NULL) < 0) {
		cerr << "Failed to open Zenoh session!" << endl;
		return -1;
	}

	HINSTANCE hInstance = GetModuleHandle(NULL);
	WNDCLASS wc = { 0 };
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = "OpenEaaglesRadarClass";
	RegisterClass(&wc);

	HWND hwnd = CreateWindow("OpenEaaglesRadarClass", "OpenEaagles 2D Aircraft Radar",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 600, 600,
		NULL, NULL, hInstance, NULL);

	HDC hdc = GetDC(hwnd);

	PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR), 1 };
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;

	int pf = ChoosePixelFormat(hdc, &pfd);
	SetPixelFormat(hdc, pf, &pfd);

	HGLRC hglrc = wglCreateContext(hdc);
	wglMakeCurrent(hdc, hglrc);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-3.0, 3.0, -3.0, 3.0);
	glMatrixMode(GL_MODELVIEW);
	glClearColor(0.0f, 0.05f, 0.0f, 1.0f);

	MSG msg = { 0 };
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			updatePhysicsAndTransmit(0.033);
			drawRadarDisplay();
			SwapBuffers(hdc);
			Sleep(33); 
		}
	}

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hglrc);
	ReleaseDC(hwnd, hdc);

	z_drop(z_move(session));
	return 0;
}