#include <SFML/Graphics.hpp>
#include <complex>
#include <utility>
#include <iostream>
#include <thread>
#include <vector>
#include <cmath>

#define _width 1024
#define _height 1024
//#define _width 512
//#define _height 512
#define _wh (_width*_height)

#define log2 (0.69314718)

#define numberOfThreads 8

using namespace std;
using namespace sf;

double DisplayTemplate[_width * _height * 4];

double scaleToWidth = 1.8;
double scaleToHeight = 1.8;
double scale = 1;
//x -0.792825716902 y 0.161256390462
//x -0.7746806106269039 y -0.1374168856037867
double x_off = -0.73640563521269319924;		//offset for x
double y_off = 0.13918372122966046756;		//offset for y

int globalIterator = 0;

complex<double> mouseComplexPosition;

thread Threads[numberOfThreads];

Vector2i lastMousePosition(_width / 2, _height / 2);

Vertex line[1000];

int c_off = 137;				//color offset

//int		goesToInfinity = (1 << 16);

//==============Fractal Const==============
int		goesToInfinity = 2;
int		belongsToSet = 600;
double	highiestIteration = 0;
double	lowestIteration = 99999;
//============================

bool	drawMandelbrot = false;
bool	cIsUpdating = true;
bool	permanentMandelbrot = true;
bool	drawFrequency = false;

Color HsvToRgb(Uint8 h, Uint8 s, Uint8 v) {
	Color rgb(0, 0, 0);
	unsigned char region, remainder, p, q, t;
	if (s == 0) {
		rgb.r = v;
		rgb.g = v;
		rgb.b = v;
		return rgb;
	}
	region = h / 43;
	remainder = (h - (region * 43)) * 6;
	p = (v * (255 - s)) >> 8;
	q = (v * (255 - ((s * remainder) >> 8))) >> 8;
	t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;
	switch (region) {
	case 0:
		rgb.r = v; rgb.g = t; rgb.b = p;
		break;
	case 1:
		rgb.r = q; rgb.g = v; rgb.b = p;
		break;
	case 2:
		rgb.r = p; rgb.g = v; rgb.b = t;
		break;
	case 3:
		rgb.r = p; rgb.g = q; rgb.b = v;
		break;
	case 4:
		rgb.r = t; rgb.g = p; rgb.b = v;
		break;
	default:
		rgb.r = v; rgb.g = p; rgb.b = q;
		break;
	}
	return rgb;
}

inline Color getColor(double a) {
	/*if (a != -1) {
		a = (a - lowestIteration) / highiestIteration;
	}*/
	double c1 = floor(a);
	double c2 = floor(a) + 1;
	double intp;

	Color ret = HsvToRgb(c1 + (modf(a, &intp) * (c2 - c1)), 200, 200 * (a != -1));
	return ret;
}

void setPixel(int x, int y, Color c, Uint8 px[]) {
	px[_width * y * 4 + x * 4] = c.r;
	px[_width * y * 4 + x * 4 + 1] = c.g;
	px[_width * y * 4 + x * 4 + 2] = c.b;
	px[_width * y * 4 + x * 4 + 3] = c.a;
}

inline complex<double> getcomplex(int x, int y) {
	complex<double> ret((x - (_width / 2.)) / (_width / 2.) * (scaleToWidth * scale) + x_off, (y - (_height / 2.)) / (_height / 2.) * (scaleToHeight * scale) + y_off);
	return ret;
}

int undocomplex(double x, double off, int w, double sck) {
	return ((w / 2) * (1 + (x - off) / (sck * scale)));
}

void MinMax(complex<double> mouseComplex, int start, int end, double f(complex<double>, complex<double>)) { //mouse position, start, stop, generator function f(point of interest, depends on fractal)
	highiestIteration = 0;
	lowestIteration = 99999;
	for (int i = start; i < end; i++) {
		int x = i % _width;
		int y = i / _width;
		double t = f(getcomplex(x, y), mouseComplex);
		DisplayTemplate[i] = t;
		highiestIteration = max(highiestIteration, t);
		if (t != -1)
			lowestIteration = min(lowestIteration, t);
	}
}

void GenerateImage(int start, int end, Uint8 px[]) { //start, stop, output array
	for (int i = start; i < end; i++) {
		int x = i % _width;
		int y = i / _width;
		Color temp = getColor(DisplayTemplate[i]);
		setPixel(x, y, temp, px);
	}
}

vector<Image> save;

Uint8 TemporaryPixelBuffer[_width * _height * 4];
void MultiThreadedGenerator(double f(complex<double>, complex<double>), complex<double> mouseComplex, Texture& output) { //generating function, mouse position, exit
	for (int i = 0; i < numberOfThreads; i++) {
		Threads[i] = thread(MinMax, mouseComplex, _wh * i / numberOfThreads, _wh * (i + 1) / numberOfThreads, f);
	}
	for (int i = 0; i < numberOfThreads; i++) {
		Threads[i].join();
	}
	for (int i = 0; i < numberOfThreads; i++) {
		Threads[i] = thread(GenerateImage, _wh * i / numberOfThreads, _wh * (i + 1) / numberOfThreads, TemporaryPixelBuffer);
	}
	for (int i = 0; i < numberOfThreads; i++) {
		Threads[i].join();
	}

	Image img;
	img.create(_width, _height, TemporaryPixelBuffer);
	save.push_back(img);


	//when generating files this should be removed for optimization 
	output.update(TemporaryPixelBuffer);
}

//==================== Fractal Generator ==========================
double MandelBrot(complex<double> c, complex<double> useless) {
	//complex<double> z(0., 0.);
	double R = 0.;
	double I = 0.;
	double Rc = real(c);
	double Ic = imag(c);
	double nR = 0.;
	double nI = 0.;
	double ret = -1;
	for (int i = 0; i < belongsToSet; i++) {
		nR = R * R - I * I;
		nI = 2 * R * I;
		nR += Rc;
		nI += Ic;
		R = nR;
		I = nI;
		if (R * R + I * I > goesToInfinity) {
			ret = i + 1. - log((log(R * R + I * I) / 2) / log2) / log2;
			//ret = i;
			break;
		}
	}
	return ret;
}

double JuliaSet(complex<double> z, complex<double> c) {

	double R = real(z);
	double I = imag(z);
	double Rc = real(c);
	double Ic = imag(c);
	double nR = 0.;
	double nI = 0.;
	double ret = -1;
	for (int i = 0; i < belongsToSet; i++) {
		nR = R * R - I * I;
		nI = 2 * R * I;
		nR += Rc;
		nI += Ic;
		R = nR;
		I = nI;
		if (R * R + I * I > goesToInfinity) {
			ret = i + 1. - log((log(R * R + I * I) / 2) / log2) / log2;
			//ret = i;
			break;
		}
	}
	return ret;
}

//====================================================================

int main() {
	//printf("%.8lf", log(2));
	Text tx;									//Text object
	Font font;									//
	font.loadFromFile("font.ttf");				//

	Texture	MainBuffer;							//Stores pixels on the GPU for main image
	MainBuffer.create(_width, _height);			//
	Sprite	MainDisplay(MainBuffer);			//Main image being displayed

	Texture MandelbrotBuffer;					//Stores pixels on reference MandelbrotSet
	MandelbrotBuffer.create(_width, _height);	//
	Sprite MandelbrotDisplay(MandelbrotBuffer);	//Mandelbrot biung displayed

	RenderWindow window(VideoMode(_width, _height), "Fraktal_SFML");


	//output to file insted of canvas

	/*for (; globalIterator < 1000; globalIterator++) {
		if (!(globalIterator % 10))
			cout << globalIterator / 10 << "\n";
		scale *= 1 / 1.04;
		MultiThreadedGenerator(MandelBrot, complex<double>(0, 0), MandelbrotBuffer);
	}
	for (int i = 0; i < save.size(); i++) {
		string path = "images1\\set_" + to_string(i) + ".bmp";
		save.at(i).saveToFile(path);
	}
	return 0;*/

	MultiThreadedGenerator(JuliaSet, complex<double>(0, 0), MandelbrotBuffer);  //function that generates chosen fractal
	while (window.isOpen()) {
		Event event;
		Vector2i mousePosition = Mouse::getPosition(window);
		complex<double> tempComplexMousePosition = getcomplex(mousePosition.x, mousePosition.y);
		if (cIsUpdating)
			mouseComplexPosition = getcomplex(mousePosition.x, mousePosition.y);

		while (window.pollEvent(event)) {
			switch (event.type) {
			case Event::Closed:
				window.close();
				break;

			case Event::MouseWheelScrolled:
				scale *= pow(1.4, -1 * event.mouseWheelScroll.delta);

				if (cIsUpdating)
					mouseComplexPosition = getcomplex(mousePosition.x, mousePosition.y);

				MultiThreadedGenerator(MandelBrot, complex<double>(0, 0), MandelbrotBuffer);
				MultiThreadedGenerator(JuliaSet, mouseComplexPosition, MainBuffer);
				break;

			case Event::MouseButtonPressed:

				if (drawFrequency) {
					double sx = real(tempComplexMousePosition);
					double sy = imag(tempComplexMousePosition);
					double nx;
					double ny;
					line[0] = Vertex(Vector2f(undocomplex(sx, x_off, _width, scaleToWidth), undocomplex(sy, y_off, _height, scaleToHeight)));
					cout << undocomplex(sx, x_off, _width, scaleToWidth) << " " << undocomplex(sy, y_off, _height, scaleToHeight) << "\n";
					//cout << undocomplex(sx, x_off, _width, scaleToWidth) << "\n";
					for (int i = 1; i < 1000; i++) {
						nx = sx * sx - sy * sy;
						ny = 2 * sx * sy;
						line[i] = Vertex(Vector2f(undocomplex(nx, x_off, _width, scaleToWidth), undocomplex(ny, y_off, _height, scaleToHeight)));
						sx = nx;
						sy = ny;
					}
				}
				else {
					x_off = real(tempComplexMousePosition);
					y_off = imag(tempComplexMousePosition);

					if (cIsUpdating)
						mouseComplexPosition = getcomplex(mousePosition.x, mousePosition.y);

					MultiThreadedGenerator(MandelBrot, complex<double>(0, 0), MandelbrotBuffer);
					MultiThreadedGenerator(JuliaSet, mouseComplexPosition, MainBuffer);
				}
				break;

			case Event::KeyPressed:
				switch (event.key.code) {
				case Keyboard::Space:
					drawMandelbrot = true; //In Julia draw shade of underlying mandelbrot
					break;
				case Keyboard::P:
					permanentMandelbrot = !permanentMandelbrot; //In Julia Permanently draw mandelbrot
					break;
				case Keyboard::F:
					cIsUpdating = !cIsUpdating; //In Julia stop updating seed number
					break;
				case Keyboard::G:
					drawFrequency = !drawFrequency; //draw path of mandelbroth transformation
					break;
				case Keyboard::S:
					printf("xoff: %.20lf\ntoff: %.20lf", x_off, y_off);
				case Keyboard::Escape:
					return 0;
				default:
					break;
				}
				break;

			case Event::KeyReleased:
				switch (event.key.code) {
				case Keyboard::Space:
					drawMandelbrot = false;
					break;
				default:
					break;
				}
				break;

			default:
				break;
			}
		}



		tx.setString("c = " + to_string(real(mouseComplexPosition)) + " + " + to_string(imag(mouseComplexPosition)) + "i ");
		tx.setCharacterSize(28);
		tx.setFillColor(Color::White);
		tx.setFont(font);

		window.clear();

		if (lastMousePosition != mousePosition && cIsUpdating && !permanentMandelbrot) {
			MultiThreadedGenerator(JuliaSet, mouseComplexPosition, MainBuffer);
		}
		lastMousePosition = mousePosition;

		window.draw(MainDisplay);

		if (drawMandelbrot || permanentMandelbrot) {
			MandelbrotDisplay.setColor(Color(255, 255, 255, 120 + 135 * (permanentMandelbrot == 1)));
			window.draw(MandelbrotDisplay);
		}

		if (drawFrequency) {
			window.draw(line, 1000, LinesStrip);
		}
		window.draw(tx);

		window.display();
	}

	return 0;
}