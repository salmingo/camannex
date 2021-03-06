/**
 Name        : AMath.cpp 常用数学函数定义文件
 Author      : Xiaomeng Lu
 Version     : 0.1
 Date        : Oct 13, 2012
 Last Date   : Oct 13, 2012
 Description : 天文数字图像处理中常用的数学函数
 **/

#include <stdlib.h>
#include <string.h>
#include "ADefine.h"
#include "AMath.h"

namespace AstroUtil
{
/*---------------------------------------------------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*------------------------------- 部分球坐标转换 -------------------------------*/
double SphereRange(double alpha1, double beta1, double alpha2, double beta2)
{
	double v = cos(beta1) * cos(beta2) * cos(alpha1 - alpha2) + sin(beta1) * sin(beta2);
	return acos(v);
}

void Sphere2Cart(double r, double alpha, double beta, double& x, double& y, double& z)
{
	x = r * cos(beta) * cos(alpha);
	y = r * cos(beta) * sin(alpha);
	z = r * sin(beta);
}

void Cart2Sphere(double x, double y, double z, double& r, double& alpha, double& beta)
{
	r = sqrt(x * x + y * y + z * z);
	if (fabs(y) < AEPS && fabs(x) < AEPS) alpha = 0;
	else if ((alpha = atan2(y, x)) < 0) alpha += A2PI;
	beta  = atan2(z, sqrt(x * x + y * y));
}

void RotX(double fai, double &x, double &y, double &z) {
	double y1, z1;
	double cf(cos(fai)), sf(sin(fai));
	y1 =  cf * y + sf * z;
	z1 = -sf * y + cf * z;
	y = y1;
	z = z1;
}

void RotY(double fai, double &x, double &y, double &z) {
	double x1, z1;
	double cf(cos(fai)), sf(sin(fai));
	x1 = cf * x - sf * z;
	z1 = sf * x + cf * z;
	x = x1;
	z = z1;
}

void RotZ(double fai, double &x, double &y, double &z) {
	double x1, y1;
	double cf(cos(fai)), sf(sin(fai));
	x1 =  cf * x + sf * y;
	y1 = -sf * x + cf * y;
	x = x1;
	y = y1;
}

void RotateForward(double A0, double D0, double A, double D, double& xi, double& eta)
{
	double r = 1.0;
	double x, y, z;	// 直角坐标位置

	// 在原坐标系的球坐标转换为直角坐标
	Sphere2Cart(r, A, D, x, y, z);
	/*! 对直角坐标做旋转变换. 定义矢量V=(alpha0, beta0)
	 * 被动视角, 旋转矢量V
	 * 先绕Z轴逆时针旋转: alpha0, 将矢量V旋转至XZ平面
	 * 再绕Y轴逆时针旋转: (PI90 - beta0), 将矢量V旋转至与Z轴重合
	 **/
	RotZ(A0, x, y, z);
	RotY(API * 0.5 - D0, x, y, z);
	// 将旋转变换后的直角坐标转换为球坐标, 即以(alpha0, beta0)为极轴的新球坐标系中的位置
	Cart2Sphere(x, y, z, r, xi, eta);
}

void RotateReverse(double A0, double D0, double xi, double eta, double& A, double& D)
{
	double r = 1.0;
	double x, y, z;

	// 在新坐标系的球坐标转换为直角坐标
	Sphere2Cart(r, xi, eta, x, y, z);
	/*! 对直角坐标做旋转变换.  定义矢量V=(alpha0, beta0)
	 * 被动旋转, 旋转矢量V
	 * 先绕Y轴逆时针旋转: -(PI90 - beta0)
	 * 再绕Z轴逆时针旋转: -alpha0
	 **/
	RotY(-API * 0.5 + D0, x, y, z);
	RotZ(-A0, x, y, z);
	// 将旋转变换后的直角坐标转换为球坐标
	Cart2Sphere(x, y, z, r, A, D);
}

// 球坐标投影到平面坐标
void ProjectForward(double A0, double D0, double A, double D, double &ksi, double &eta)
{
	double fract = sin(D0) * sin(D) + cos(D0) * cos(D) * cos(A - A0);
	ksi = cos(D) * sin(A - A0) / fract;
	eta = (cos(D0) * sin(D) - sin(D0) * cos(D) * cos(A - A0)) / fract;
}

// 被投影的平面坐标转换到球坐标
void ProjectReverse(double A0, double D0, double ksi, double eta, double &A, double &D)
{
	double fract = cos(D0) - eta * sin(D0);
	A = A0 + atan2(ksi, fract);
	D = atan(((eta * cos(D0) + sin(D0)) * cos(A - A0)) / fract);
	if(A < 0) A += A2PI;
	else if(A >= A2PI) A -= A2PI;
}
/*------------------------------- 部分球坐标转换 -------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*-------------------------------- 部分矩阵转换 --------------------------------*/
/*
 * @note 矩阵乘积
 */
void MatRealMult(int M, int L, int N, double A[], double B[], double RM[]) {
	double z, *q0, *p, *q;
	int i, j, k;

	q0 = (double *)calloc(L, sizeof(double));
	for(i = 0; i < N; ++i, ++RM) {
		for(k = 0, p = B + i; k < L; ++k, p += N) q0[k] = *p;
		for(j = 0, p = A, q = RM; j < M; ++j, q += N) {
			for(k = 0, z = 0.; k < L; ++k, ++p) z += *p * q0[k];
			*q = z;
		}
	}

	free(q0);
}

/*
 * @note 转置矩阵
 */
void MatTrans(int M, int N, double A[], double B[]) {
	int i, j;
	double *p, *q;

	for(i = 0, q = B; i < N; ++i, ++A) {
		for(j = 0, p = A; j < M; ++j, p += N, ++q) *q = *p;
	}
}

/*
 * @note 逆矩阵
 */
int MatInvert(int N, double A[]) {
	int lc;
	int *le, *lep;
	double s,t,tq = 0., zr = 1.e-15;
	double *pa, *pd, *ps, *p, *q;
	double *q0;
	int i, j, k, m;

	le = lep = (int *) malloc(N * sizeof(int));
	q0 = (double *) malloc(N * sizeof(double));

	for(j = 0, pa = pd = A; j < N ; ++j, ++pa, pd += N + 1) {
		if( j > 0) {
			for(i = 0, q = q0, p = pa; i < N; ++i, ++q, p += N) *q = *p;
			for(i = 1; i < N; ++i) {
				lc = i < j ? i : j;
				for(k = 0, p = pa + i * N - j, q = q0,t = 0.; k < lc ;++k, ++p, ++q) t += *p * *q;
				q0[i] -= t;
			}
			for(i = 0, q = q0, p = pa; i < N; ++i, p += N, ++q) *p = *q;
		}

		s = fabs(*pd);
		lc = j;

		for(k = j + 1, ps = pd; k < N; ++k) {
			if((t = fabs(*(ps += N))) > s) {
				s = t;
				lc = k;
			}
		}

		tq = tq > s ? tq : s;
		if(s < zr * tq) {
			free(q0);
			free(le);
			return -1;
		}

		*lep++ = lc;
		if(lc != j) {
			for(k = 0, p = A + N * j, q = A + N * lc; k < N; ++k, ++p, ++q) {
				t = *p;
				*p = *q;
				*q = t;
			}
		}

		for(k = j + 1, ps = pd, t=1./ *pd; k < N; ++k) *(ps += N) *= t;
		*pd = t;
	}

	for(j = 1, pd = ps = A; j < N; ++j) {
		for(k = 0, pd += N + 1, q = ++ps; k < j; ++k, q += N) *q *= *pd;
	}

	for(j = 1, pa = A; j < N; ++j) {
		++pa;
		for(i = 0, q = q0, p = pa; i < j; ++i, p += N, ++q) *q = *p;
		for(k = 0; k < j; ++k) {
			t=0.;
			for(i = k, p = pa + k * N + k - j, q = q0 + k; i<j; ++i, ++p, ++q) t -= *p * *q;
			q0[k] = t;
		}
		for(i = 0, q = q0, p = pa; i < j; ++i, p += N, ++q) *p = *q;
	}

	for(j = N - 2, pd = pa = A + N * N - 1; j >= 0; --j) {
		--pa;
		pd -= N + 1;

		for(i = 0, m = N - j - 1, q = q0, p = pd + N; i < m; ++i, p += N, ++q) *q = *p;
		for(k = N - 1, ps = pa; k > j; --k, ps -= N) {
			t = -(*ps);
			for(i = j + 1, p = ps, q = q0; i < k; ++i, ++q) t -= *++p * *q;
			q0[--m] = t;
		}
		for(i = 0, m = N - j - 1, q = q0, p = pd + N; i < m; ++i, p += N) *p = *q++;
	}

	for(k = 0, pa = A; k < N - 1; ++k, ++pa) {
		for(i = 0, q = q0, p = pa; i < N; ++i, p += N) *q++ = *p;
		for(j = 0, ps =A; j < N; ++j, ps += N) {
			if(j > k) { t = 0.;    p = ps + j;     i = j; }
			else      { t = q0[j]; p = ps + k + 1; i = k + 1; }
			for(; i < N;) t += *p++ * q0[i++];
			q0[j] = t;
		}

		for(i = 0, q = q0, p = pa; i < N; ++i, p += N) *p = *q++;
	}

	for(j = N - 2, --lep; j >= 0;--j) {
		for(k = 0, p = A + j, q = A + *(--lep); k < N; ++k, p += N, q += N) {
			t = *p;
			*p = *q;
			*q = t;
		}
	}

	free(le);
	free(q0);
	return 0;
}

/* 线性最小二乘拟合 */
bool LSLinearSolve(int M, double Y[], double A[], double X[])
{
	if(MatInvert(M, A)) return false;
	MatRealMult(M, M, 1, A, Y, X);
	return true;
}

/* 高斯曲面拟合 */
bool GaussFit2D(int N, double X[], double Y[], double Z[], double& A, double &sigmax, double& sigmay, double& x0, double& y0)
{
	double Q[5], B[25], C[5];
	double x, y, z, zx, zy, zx2, zy2, zlnz;
	int i;

	memset(Q, 0, sizeof(double) * 5);
	memset(B, 0, sizeof(double) * 25);

	for(i = 0; i < N; i++) {
		x = X[i];
		y = Y[i];
		z = Z[i];
		if(z <= 1E-10) continue;	// 如果z<=0则剔除该点
		zx   = z * x;
		zy   = z * y;
		zx2  = zx * x;
		zy2  = zy * y;
		zlnz = z * log(z);

		Q[0] += zlnz * z;
		Q[1] += zlnz * zx;
		Q[2] += zlnz * zy;
		Q[3] += zlnz * zx2;
		Q[4] += zlnz * zy2;

		B[0]  += z * z;
		B[1]  += z * zx;
		B[2]  += z * zy;
		B[3]  += z * zx2;
		B[4]  += z * zy2;

		B[5]  += zx * z;
		B[6]  += zx * zx;
		B[7]  += zx * zy;
		B[8]  += zx * zx2;
		B[9]  += zx * zy2;

		B[10] += zy * z;
		B[11] += zy * zx;
		B[12] += zy * zy;
		B[13] += zy * zx2;
		B[14] += zy * zy2;

		B[15] += zx2 * z;
		B[16] += zx2 * zx;
		B[17] += zx2 * zy;
		B[18] += zx2 * zx2;
		B[19] += zx2 * zy2;

		B[20] += zy2 * z;
		B[21] += zy2 * zx;
		B[22] += zy2 * zy;
		B[23] += zy2 * zx2;
		B[24] += zy2 * zy2;
	}

	if(!LSLinearSolve(5, Q, B, C)) return false;		// 如果B是奇异矩阵
	if(C[3] >= -1E-10 || C[4] >= -1E-10) return false;	// 如果C[3]或C[4]>=0
	A     = exp(C[0] - C[1] * C[1] / C[3] / 4 - C[2] * C[2] / C[4] / 4);
	x0    = -0.5 * C[1] / C[3];
	y0    = -0.5 * C[2] / C[4];
	sigmax= sqrt(-0.5 / C[3]);
	sigmay= sqrt(-0.5 / C[4]);
	return true;
}
/*-------------------------------- 部分矩阵转换 --------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*------------------------------- 大小端数据转换 -------------------------------*/
bool TestSwapEndian()
{
	short v = 0x0102;
	char* p = (char*) &v;
	return (p[0] == 2);
}

void SwapEndian(void* array, int nelement, int ncell)
{
	if (ncell % 2 != 0) return;	// 不满足转换条件
	int np = ncell - 1;
	char *p, *e, *p1, *p2;
	char ch;

	for (p = (char*) array, e = p + ncell * nelement; p < e; p += ncell) {
		for (p1 = p, p2 = p1 + np; p1 < p2; p1++, p2--) {
			ch = *p1;
			*p1 = *p2;
			*p2 = ch;
		}
	}
}
/*------------------------------- 大小端数据转换 -------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*----------------------------------- 内插 -----------------------------------*/
void spline(int n, double x[], double y[], double c1, double cn, double c[])
{
	int i;
	double p, qn, sig, un, t1, t2;
	double* u = (double*) calloc(n, sizeof(double));
	double limit = 0.99 * AMAX;
	if (c1 > limit) {
		c[0] = 0;
		u[0] = 0;
	}
	else {
		c[0] = -0.5;
		u[0] = (3 / (x[1] - x[0])) * (y[1] - y[0]) / (y[1] - y[0] - c1);
	}
	for (i = 1; i < n - 1; ++i) {
		sig = (t1 = x[i] - x[i - 1]) / (t2 = x[i + 1] - x[i - 1]);
		p = sig * c[i - 1] + 2;
		c[i] = (sig - 1) / p;
		u[i]=(6.*((y[i+1]-y[i])/(x[i+1]-x[i])-(y[i]-y[i-1])/t1)/t2-sig*u[i-1])/p;
	}
	if (cn > limit) {
		qn = 0;
		un = 0;
	}
	else {
		qn = 0.5;
		un = 3./(x[n-1]-x[n-2])*(cn-(y[n-1]-y[n-2])/(x[n-1]-x[n-2]));
	}
	c[n-1]=(un-qn*u[n-2])/(qn*c[n-2]+1.);
	for (i = n - 2; i >= 0; --i) {
		c[i] = c[i] * c[i + 1] + u[i];
	}
	free(u);
}

bool splint(int n, double x[], double y[], double c[], double xo, double& yo)
{
	int k, khi, klo;
	double a, b, h;

	klo = 0;
	khi = n - 1;
	while ((khi - klo) > 1) {
		k = (khi + klo) / 2;
		if (x[k] > xo) khi = k;
		else           klo = k;
	}
	h = x[khi] - x[klo];
	if (fabs(h) < AEPS) return false;
	a = (x[khi] - xo) / h;
	b = (xo - x[klo]) / h;
	yo = a * y[klo] + b * y[khi] + ((a * a - 1) * a * c[klo] + (b * b - 1) * b * c[khi]) * h * h / 6;

	return true;
}

void spline2(int nr, int nc, double x1[], double x2[], double y[], double c[])
{
	int i, j, k;
	double* ytmp = (double*) calloc(nc, sizeof(double));
	double* ctmp= (double*) calloc(nc, sizeof(double));

	for (j = 0; j < nr; ++j) {
		for (k = 0, i = j * nc; k < nc; ++k, ++i) ytmp[k] = y[i];
		spline(nc, x2, ytmp, AMAX, AMAX, ctmp);
		for (k = 0, i = j * nc; k < nc; ++k, ++i) c[i] = ctmp[k];
	}

	free(ytmp);
	free(ctmp);
}

bool splint2(int nr, int nc, double x1[], double x2[], double y[], double c[], double x1o, double x2o, double& yo)
{
	bool bret = true;
	int i, j, k;
	double* ctmp  = (double*) calloc(nr > nc ? nr : nc, sizeof(double));
	double* ytmp  = (double*) calloc(nc, sizeof(double));
	double* yytmp = (double*) calloc(nr, sizeof(double));

	for (j = 0; j < nr; ++j) {
		for (k = 0, i = j * nc; k < nc; ++k, ++i) {
			ytmp[k] = y[i];
			ctmp[k] = c[i];
		}
		if (!(bret = splint(nc, x2, ytmp, ctmp, x2o, yytmp[j]))) break;
	}
	if (bret) {
		spline(nr, x1, yytmp, AMAX, AMAX, ctmp);
		bret = splint(nr, x1, yytmp, ctmp, x1o, yo);
	}

	free(ctmp);
	free(ytmp);
	free(yytmp);

	return bret;
}

void CubicSpline(int N, double XI[], double YI[], int M, double XO[], double YO[])
{
	double *A = new double[N];
	double *B = new double[N];
	double *C = new double[N];
	double *D = new double[N];
	const int L = N - 1;
	int i;
	int j;
	int k;	// = j + 1
	double t, p;

	// 边界条件
	A[0] = 0, A[L] = 1;
	B[0] = 1, B[L] = -1;
	C[0] = -1, C[L] = 0;
	D[0] = 0, D[L] = 0;
	// 构建系数
	for(i = 1; i < L; ++i) {
		t = XI[i] - XI[i - 1];
		p = XI[i + 1] - XI[i];
		A[i] = t / 6;
		C[i] = p / 6;
		B[i] = 2 * (A[i] + C[i]);
		D[i] = (YI[i + 1] - YI[i]) / p - (YI[i] - YI[i - 1]) / t;
	}
	for(i = 1; i < N; ++i) {
		t = B[i] - A[i] * C[i - 1];
		C[i] = C[i] / t;
		D[i] = (D[i] - A[i] * D[i - 1]) / t;
	}
	A[N - 1] = D[N - 1];
	for(i = 0; i < L; ++i) {
		j = N - i - 2;
		A[j] = D[j] - C[j] * A[j + 1];
	}

	// 内插拟合
	for(i = 0; i < M; ++i) {
		t = XO[i];
		// 添加限制判断, 在接近两端也获得可接受的结果
		if (t < XI[0]) j = 0;
		else if (t > XI[L]) j = L - 1;
		else {
			for(j = 0; j < L; ++j) {
				if(XI[j] <= t && t <= XI[j + 1]) break;
			}
		}
		k = j + 1;
		p = XI[k] - XI[j];
		YO[i] = (A[j] * pow(XI[k] - t, 3.) + A[k] * pow(t - XI[j], 3.)) / 6 / p
			+ (XI[k] - t) * (YI[j] / p - A[j] * p / 6) + (t - XI[j]) * (YI[k] / p - A[k] * p / 6);
	}

	delete []A;
	delete []B;
	delete []C;
	delete []D;
}

double Bilinear(double XI[], double YI[], double ZI[], double X0, double Y0)
{
	double f1 = ZI[0] * (XI[1] - X0) * (YI[1] - Y0);
	double f2 = ZI[1] * (X0 - XI[0]) * (YI[1] - Y0);
	double f3 = ZI[2] * (XI[1] - X0) * (Y0 - YI[0]);
	double f4 = ZI[3] * (X0 - XI[0]) * (Y0 - YI[0]);

	return (f1 + f2 + f3 + f4) / (XI[1] - XI[0]) / (YI[1] - YI[0]);
}

void Lagrange(int N, double XI[], double YI[], int OD, int M, double XO[], double YO[])
{
	if (OD < 2) OD = 2;	// OD＝2时等效线性内插
	if (OD > N) OD = N;	// OD＝N时等效所有已知数据参与内插
	int OH = OD / 2;
	int SI, EI;			// SI: 参与内插的起始位置; EI: 参与内插的结束位置
	int i, j, k;
	double t0, t1;
	double x;

	for (k = 0; k < M; ++k) {// 逐点进行内插
		x = XO[k];
		// 查找最邻近的大于x的起始位置
		for (j = 0; j < N; ++j) {
			if (XI[j] > x) break;
		}
		// 检查参与内插的位置
		SI = j - OH;
		EI = SI + OD - 1;
		if (SI < 0) {
			SI = 0;
			EI = OD - 1;
		}
		if (EI >= N) {
			EI = N - 1;
			SI = EI - OD + 1;
		}
		// 内插
		t0 = 0;
		for (i = SI; i <= EI; ++i) {
			t1 = 1;
			for (j = SI; j < i; ++j) t1 = t1 * (x - XI[j]) / (XI[i] - XI[j]);
			for (j = i + 1; j <= EI; ++j) t1 = t1 * (x - XI[j]) / (XI[i] - XI[j]);
			t0 = t0 + t1 * YI[i];
		}
		YO[k] = t0;
	}
}
/*----------------------------------- 内插 -----------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*--------------------------------- 相关系数 ---------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*------------------------------- 部分星等转换 -------------------------------*/
double Sr2Arcsec(double sr)
{
	return (sr * R2S * R2S);
}

double Arcsec2Sr(double sas)
{
	return (sas / R2S / R2S);
}

double Mag2Watt(double mag)
{
	return (1.78E-8 * pow(10., -0.4 * mag));
}

double Watt2Mag(double watt)
{
	return (-2.5 * log10(1E8 * watt / 1.78));
}

double Candela2Watt(double cd)
{
	return (10000. * cd / 683.);
}

double Watt2Candela(double watt)
{
	return (watt * 683. / 10000.);
}

double Mag2Photo(double mag, double wl)
{
	double h = 6.626176 * 1E-34; // 普朗克常数, [J.s]
	double c = 3.0 * 1E8;		 // 光速, [m/s]
	double watt = Mag2Watt(mag);
	double f = c / wl * 1E9;	 // 频率
	return (watt / h / f);
}

double Photo2Mag(double photo, double wl)
{
	double h = 6.626176 * 1E-34;
	double c = 3.0 * 1E8;
	double f = c / wl * 1E9;
	double watt = photo * h * f;
	return Watt2Mag(watt);
}
/*------------------------------- 部分星等转换 -------------------------------*/
///////////////////////////////////////////////////////////////////////////////
double Correlation(int N, double X[], double Y[])
{
	double sumx = 0;
	double sumy = 0;
	double sqx = 0;
	double sqy = 0;
	double sumxy = 0;
	double meanx, meany, rmsx, rmsy;
	int i;

	for (i = 0; i < N; ++i) {
		sumx += X[i];
		sumy += Y[i];
		sumxy += X[i] * Y[i];
		sqx += X[i] * X[i];
		sqy += Y[i] * Y[i];
	}
	meanx = sumx / N;
	meany = sumy / N;
	rmsx = sqrt((sqx - sumx * meanx) / (N - 1));
	rmsy = sqrt((sqy - sumy * meany) / (N - 1));
	return (sumxy - N * meanx * meany) / rmsx / rmsy;
}
/*--------------------------------- 相关系数 ---------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*@
 * 开发排序算法是为了减少固化时对开发库的依赖
 */
/*---------------------------------- 排序算法 ----------------------------------*/
/*---------------------------------- 排序算法 ----------------------------------*/
/*--------------------------------- 误差系数 ---------------------------------*/
double erf(double x)
{
    double t, z, y;
    z = fabs(x);
    t = 1.0 / (1.0 + 0.5 * z);
    y = t * exp(-z * z - 1.26551223 + t * ( 1.00002368 + t * ( 0.37409196 + t *
               ( 0.09678418 + t * (-0.18628806 + t * ( 0.27886807 + t *
               (-1.13520398 + t * ( 1.48851587 + t * (-0.82215223 + t * 0.17087277)))))))));
    if (x >= 0.0) y = 1.0 - y;
    else        y = y - 1.0;
    return y;
}

double reverse_erf(double z)
{
    double lo = -4;    // 默认对应-1
    double hi =  4;    // 默认对应+1
    if (z >= 0) lo = 0;
    else        hi = 0;
    double me = (lo + hi) * 0.5;
    double z1 = erf(me);
    int i = 1;
    while (fabs(z1 - z) > 1.2E-7) {
        if (z1 < z) lo = me;
        else        hi = me;
        me = (lo + hi) * 0.5;
        z1 = erf(me);
        ++i;
    }
    return me;
}

double CNDF(double x, double mu, double sigma)
{
    double z = (x - mu) / sqrt(2.0) / sigma;
    return 0.5 * (1 + erf(z));
}

double RCNDF(double p, double mu, double sigma)
{
    double z = reverse_erf(p * 2 - 1);
    double x = z * sqrt(2.0) * sigma + mu;
    return x;
}
/*--------------------------------- 误差系数 ---------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------*/
}
