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

struct Aircraft { double x = -2.5, y = -2.5, speedKts = 300.0, headingDeg = 45.0, altFt = 15000.0; } ac;
struct Radar { double x = 0.0, y = 0.0, rangeNm = 3.0; } radar;
struct SAM { double x = 1.0, y = 1.0, rangeNm = 1.5; bool tracking = false; } sam;

int simulationTimeMs = 0;

void updatePhysicsAndTransmit(double dtSec) {
	double distNM = (ac.speedKts / 3600.0) * dtSec;
	double headingRad = ac.headingDeg * (M_PI / 180.0);
	ac.x += distNM * sin(headingRad);
	ac.y += distNM * cos(headingRad);
	
	if (ac.x > 3.0 || ac.y > 3.0) {
		ac.x = -2.5; ac.y = -2.5;
	}

	double distToSam = sqrt(pow(ac.x - sam.x, 2) + pow(ac.y - sam.y, 2));
	sam.tracking = (distToSam <= sam.rangeNm);

	simulationTimeMs += static_cast<int>(dtSec * 1000.0);

	stringstream ss;
	ss << fixed << setprecision(3);
	ss << "{"
		<< "\"time_ms\":" << simulationTimeMs << ","
		<< "\"aircraft\":{\"x\":" << ac.x << ",\"y\":" << ac.y << ",\"alt\":" << ac.altFt << "},"
		<< "\"radar\":{\"x\":" << radar.x << ",\"y\":" << radar.y << ",\"range\":" << radar.rangeNm << "},"
		<< "\"sam\":{\"x\":" << sam.x << ",\"y\":" << sam.y << ",\"tracking\":" << (sam.tracking ? "true" : "false") << "}"
		<< "}";

	string payloadStr = ss.str();

	z_owned_bytes_t payloadBytes;
	z_bytes_from_static_str(&payloadBytes, payloadStr.c_str());

	z_view_keyexpr_t keyExpression;
	z_view_keyexpr_from_str(&keyExpression, "simulation/state/all");

	z_put(z_loan(session), z_loan(keyExpression), z_move(payloadBytes), NULL);
	cout << "[40ms TRANSMIT] " << payloadStr << endl;
}

void drawRadarDisplay() {
	glClear(GL_COLOR_BUFFER_BIT);
	glLoadIdentity();

	glColor3f(0.0f, 0.25f, 0.0f);
	glBegin(GL_LINES);
	for (float i = -3.0f; i <= 3.0f; i += 1.0f) {
		glVertex2f(i, -3.0f); glVertex2f(i, 3.0f);
		glVertex2f(-3.0f, i); glVertex2f(3.0f, i);
	}
	glEnd();

	glColor3f(0.0f, 0.5f, 1.0f);
	glBegin(GL_LINE_LOOP);
	glVertex2f(-0.1f, -0.1f); glVertex2f(0.1f, -0.1f);
	glVertex2f(0.1f, 0.1f);   glVertex2f(-0.1f, 0.1f);
	glEnd();

	glPushMatrix();
	glTranslatef((GLfloat)sam.x, (GLfloat)sam.y, 0.0f);
	if (sam.tracking) glColor3f(1.0f, 0.0f, 0.0f);
	else glColor3f(1.0f, 1.0f, 0.0f);

	glBegin(GL_TRIANGLES);
	glVertex2f(0.0f, 0.12f);
	glVertex2f(-0.1f, -0.08f);
	glVertex2f(0.1f, -0.08f);
	glEnd();
	glPopMatrix();
	glPushMatrix();
	glTranslatef((GLfloat)ac.x, (GLfloat)ac.y, 0.0f);
	glColor3f(1.0f, 0.2f, 0.2f);
	glBegin(GL_POLYGON);
	glVertex2f(0.0f, 0.12f);
	glVertex2f(-0.08f, 0.0f);
	glVertex2f(0.0f, -0.08f);
	glVertex2f(0.08f, 0.0f);
	glEnd();
	glPopMatrix();
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

int main(int argc, char* argv[]) {
	cout << "======================================================" << endl;
	cout << "    TRANSMITTER: Aircraft + Radar + SAM (40ms / 25Hz) " << endl;
	cout << "======================================================" << endl;

	z_owned_config_t config;
	z_config_default(&config);
	if (z_open(&session, z_move(config), NULL) < 0) {
		cerr << "Failed to open Zenoh session!" << endl;
		return -1;
	}

	HINSTANCE hInstance = GetModuleHandle(NULL);
	WNDCLASSW wc = { 0 };
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"OpenEaaglesMultiRadarClass";
	RegisterClassW(&wc);

	HWND hwnd = CreateWindowW(L"OpenEaaglesMultiRadarClass", L"Tactical Radar Display (25 Hz)",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 600, 600, NULL, NULL, hInstance, NULL);

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
	glClearColor(0.0f, 0.02f, 0.0f, 1.0f);

	MSG msg = { 0 };
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			updatePhysicsAndTransmit(0.040);
			drawRadarDisplay();
			SwapBuffers(hdc);
			Sleep(40);
		}
	}

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hglrc);
	ReleaseDC(hwnd, hdc);
	z_drop(z_move(session));
	return 0;
}