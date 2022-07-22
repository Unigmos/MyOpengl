/* List: p3-robot.c
 * Robot arm with two links and two joints.
 * Examination of world-local coordinates, modeling transfomation and 
 * operation of matrix stack.
 */
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

#define	imageWidth 256
#define	imageHeight 256

unsigned char texImage[imageHeight][imageWidth][3];

// Timer内で使用する変数
int 	samplingTime = 50, timer_flag = 20, target_timer = 100, final_flag = 1;

// 足
static int right_knee = -100, right_shin = -30, right_toe = 0;
static int left_knee = -80, left_shin = -30, left_toe = 0;

// 手
static int right_shoulder = -20, right_elbow = 75, right_hand = 0; // el160
static int left_shoulder = -20, left_elbow = 75, left_hand = 0;

// 体・首・頭
static int body = -90, neck = 0;
static double head = 0;

// ピストル
static int pistol_handle = 90, pistol_muzzle = 90, bullet_flag=0;
double bullet = 0;

// 移動速度(負の値で方向変化)
int direction = 3;

// 移動速度(右手)
int right_direction = 4;

// ガードレールの位置
double guardrail_pos = 10.0;

// ロボット全体のポジション・角度
double robot_x = 5.0;
double robot_z = 5.0;
double robot_rotate = 0.0;

// ロボットの原点からの距離
double r;

// スコア・カウントダウンタイマー
int score = 0;
unsigned int count_down = 60;
int final_score = 0;

// マウス書き込み許可
short int we = 1;
// マウスの初期位置・現在のマウス位置
double first_x = 0, first_y = 0, sub_x = 0, sub_y = 0;

// カメラ座標
double cam_x = 5, cam_y = 5;

// 背景色
double bg_color;

// 的の場所
int target_pos = 7;

// 光源の位置・色設定
double light_x = -1.0, light_y = -1.0, light_z = 1.0;
double light_r = 0.8, light_g = 0.8, light_b = 0.8;
double light_0_x = -5.0;

void myInit(char* progname)
{
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(500, 500);
	glutInitWindowPosition(0, 0);
	glutCreateWindow(progname);
	glClearColor(0.6, 0.6, 0.6, 1.0);
	glShadeModel(GL_FLAT);
}

void readPPMImage(char* filename)
{
	FILE* fp;
	int  ch, i;

	if (fopen_s(&fp, filename, "r") != 0) {
		fprintf(stderr, "Cannot open ppm file %s\n", filename);
		exit(1);
	}
	for (i = 0; i < 4; i++) { 						// skip four in header
		while (1) {
			if ((ch = fgetc(fp)) != '#') break;		// skip comment
			fgets((char*)texImage, 1024, fp);   	// dummy read
			while (isspace(ch)) ch = fgetc(fp);  	// skip space
		}
		while (!isspace(ch)) ch = fgetc(fp);		// read header
/* Newline or other single terminator after header may exist. */
		if (i < 3) {
			while (isspace(ch)) ch = fgetc(fp);		// skip terminator
		}
	}
	fread(texImage, 1, imageWidth * imageHeight * 3, fp);	// read RGB data
	fclose(fp);
}

void setUpTexture(void)
{
	readPPMImage("target_256x256.ppm");
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight,
		0, GL_RGB, GL_UNSIGNED_BYTE, texImage);
}

// タイマーセット及びスコア・最終スコアのリセット
void getMenu(int value) {
	switch (value)
	{
	case 1:
		count_down = 60;
		score = 0;
		final_score = 0;
		break;
	case 2:
		count_down = 40;
		score = 0;
		final_score = 0;
		break;
	case 3:
		count_down = 20;
		score = 0;
		final_score = 0;
		break;
	default:
		break;
	}
}

void mySetMenu() {
	glutCreateMenu(getMenu);
	glutAddMenuEntry("60s Start", 1);
	glutAddMenuEntry("40s Start", 2);
	glutAddMenuEntry("20s Start", 3);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}
void mySetLight()
{
	// 光源
	float light0_position[] = { light_0_x, -4.0, 1.0, 1.0 };	// point light source
	float light1_position[] = { light_x,  light_y, light_z, 1.0 };	// point light source
	float light1_ambient[] = { light_r, light_g, light_b, 1.0 }; // 環境光
	float light1_diffuse[] = { 0.5, 0.5, 0.5, 1.0 }; // 拡散光
	float light1_specular[] = { 0.75, 0.75, 0.75, 1.0 }; // 鏡面光

	/* Set up LIGHT0 which uses the default parameters except position */
	glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
	/* Set up LIGHT1 */
	glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
	glLightfv(GL_LIGHT1, GL_AMBIENT, light1_ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
	glLightfv(GL_LIGHT1, GL_SPECULAR, light1_specular);

	glEnable(GL_LIGHT0);		// enable the 0th light
	glEnable(GL_LIGHT1);		// enable the 1st light
}

// 画面描画
void myDisplay(void)
{
	float mtrl_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	float mtrl_shininess[] = { 50.0 };					// range [0,128]
	char text_des1[40] = "# 0-89:morning  90-179:day";
	char text_des2[40] = "180-269:evening  270-360:night";
	char text_des3[40] = "# CriticalHit: +3  Hit: +1";
	char text_des4[40] = "# 'WASD':Move  'Space':Shoot";
	char text[30] = "Score:";
	char text_2[30] = "Timer:";
	char text_3[30] = "Color:";
	char text_4[30] = "FINAL:";
	char *p;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);

	glMaterialfv(GL_FRONT, GL_SPECULAR, mtrl_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mtrl_shininess);

	double	tc = 1.0;
	double	p0[] = { -8.0, -4.0, 0.0 }, p1[] = { -8.0, 4.0, 0.0 },
		p2[] = { 0.0, 4.0, 0.0 }, p3[] = { 0.0, -4.0, 0.0 };

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);

	// 的の描画
	glPushMatrix();
		glRotated((double)90.0, 0.0, 1.0, 0.0);
		glTranslated((double)target_pos*-1, 5.5, 30.5);
		glBegin(GL_QUADS);
		glTexCoord2d(0.0, 0.0); glVertex3dv(p0);
		glTexCoord2d(0.0, tc); glVertex3dv(p1);
		glTexCoord2d(tc, tc); glVertex3dv(p2);
		glTexCoord2d(tc, 0.0); glVertex3dv(p3);

		glEnd();
	glPopMatrix();

	glFlush();
	glDisable(GL_TEXTURE_2D);

	// 文字描画(Colorの説明)
	glPushMatrix();
	glTranslated(0.0, 1.0, -7.0);
	glColor3d(1.0, 1.0, 1.0);

	glRasterPos2f(6.0, 7.0);
	glBitmap(0, 0, 0, 0, (int)100, (int)70, NULL);
	for (p = text_des1; *p; p++) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *p);
	}
	glPopMatrix();

	// 文字描画(Colorの説明2)
	glPushMatrix();
	glTranslated(0.5, 0.75, -7.0);
	glColor3d(1.0, 1.0, 1.0);

	glRasterPos2f(6.0, 7.0);
	glBitmap(0, 0, 0, 0, (int)100, (int)70, NULL);
	for (p = text_des2; *p; p++) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *p);
	}
	glPopMatrix();

	// 文字描画(当たり判定説明)
	glPushMatrix();
	glTranslated(0.0, 0.0, -7.0);
	glColor3d(1.0, 1.0, 1.0);

	glRasterPos2f(6.0, 7.0);
	glBitmap(0, 0, 0, 0, (int)100, (int)70, NULL);
	for (p = text_des3; *p; p++) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *p);
	}
	glPopMatrix();

	// 文字描画(操作方法説明)
	glPushMatrix();
	glTranslated(0.0, -0.5, -7.0);
	glColor3d(1.0, 1.0, 1.0);

	glRasterPos2f(6.0, 7.0);
	glBitmap(0, 0, 0, 0, (int)100, (int)70, NULL);
	for (p = text_des4; *p; p++) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *p);
	}
	glPopMatrix();

	// 文字描画(Score)
	glPushMatrix();
		glTranslated(0.0, 0.0, 1.0);
		glColor3d(1.0, 1.0, 1.0);

		glRasterPos2f(10.0, 8.0);
		sprintf_s((char*)(text + 6), 24, "%3d", score);
		glBitmap(0, 0, 0, 0, (int)100, (int)70, NULL);
		for (p = text; *p; p++) {
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
		}
	glPopMatrix();

	// 文字描画(Timer)
	glPushMatrix();
		glTranslated(0.0, 0.0, 1.0);
		glColor3d(1.0, 1.0, 1.0);

		glRasterPos2f(10.0, 9.0);
		sprintf_s((char*)(text_2 + 6), 24, "%2d", count_down);
		glBitmap(0, 0, 0, 0, (int)100, (int)70, NULL);
		for (p = text_2; *p; p++) {
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
		}
	glPopMatrix();

	// 角度描画(Test用)
	glPushMatrix();
	glTranslated(0.0, 0.0, 1.0);
	glColor3d(1.0, 1.0, 1.0);

	glRasterPos2f(10.0, 7.0);
	sprintf_s((char*)(text_3 + 6), 23, "%.1f", robot_rotate);
	glBitmap(0, 0, 0, 0, (int)100, (int)70, NULL);
	for (p = text_3; *p; p++) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
	}
	glPopMatrix();

	// 文字描画(final_score)
	glPushMatrix();
	glTranslated(0.0, 0.0, 1.0);
	glColor3d(1.0, 1.0, 1.0);

	glRasterPos2f(10.0, 6.0);
	sprintf_s((char*)(text_4 + 6), 24, "%3d", final_score);
	glBitmap(0, 0, 0, 0, (int)100, (int)70, NULL);
	for (p = text_4; *p; p++) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
	}
	glPopMatrix();


// まとめて動かす
	glPushMatrix();
	glTranslated((double)robot_x-5, 0.0, (double)robot_z-5);
	glRotated(0.0, 0.0, 1.0, 0.0);

// 右足
	glPushMatrix();
	/* 1st link */
		glRotated((double)right_knee, 0.0, 0.0, 1.0);
		glTranslated(0.8, 0.0, 0.5);
		glPushMatrix();
			glScaled(1.7, 0.9, 1.0);
			glutSolidCube(1.0);
		glPopMatrix();
	/* 2nd link */
		glTranslated(1.3, 0.0, 0.0);				//move to the end of 1st link
		glRotated((double)right_shin, 0.0, 0.0, 1.0);
		glTranslated(0.3, -0.2, 0.0);
		glPushMatrix();
			glScaled(1.5, 0.7, 0.9);
			glutSolidCube(1.0);
		glPopMatrix();
	/* 3rd link */
		glTranslated(0.7, 0.0, 0.0);				//move to the end of 2nd link
		glRotated((double)right_toe, 0.0, 0.0, 1.0);
		glTranslated(0.0, 0.0, 0.0);
		glPushMatrix();
			glScaled(0.4, 1.0, 1.0);
			glutSolidCube(1.0);
		glPopMatrix();
	glPopMatrix();

// 左足
	glPushMatrix();
	/* 1st link */
		glRotated((double)left_knee, 0.0, 0.0, 1.0);
		glTranslated(0.8, 0.0, -0.5);
		glPushMatrix();
			glScaled(1.7, 0.9, 1.0);
			glutSolidCube(1.0);
		glPopMatrix();
	/* 2nd link */
		glTranslated(1.3, 0.0, 0.0);				//move to the end of 1st link
		glRotated((double)left_shin, 0.0, 0.0, 1.0);
		glTranslated(0.3, -0.2, 0.0);
		glPushMatrix();
			glScaled(1.5, 0.7, 0.9);
			glutSolidCube(1.0);
		glPopMatrix();
	/* 3rd link */
		glTranslated(0.7, 0.0, 0.0);				//move to the end of 2nd link
		glRotated((double)left_toe, 0.0, 0.0, 1.0);
		glTranslated(0.0, 0.0, 0.0);
		glPushMatrix();
			glScaled(0.4, 1.0, 1.0);
			glutSolidCube(1.0);
		glPopMatrix();
	glPopMatrix();

// 体・首・頭
	
	glPushMatrix();
	// 1st link
		glRotated((double)body, 0.0, 0.0, 1.0);
		glTranslated(-2.0, 0.0, 0.0);
		glPushMatrix();
			glScaled(4.0, 1.6, 2.1);
			glutSolidCube(1.0);
		glPopMatrix();
	// 2nd link
		glTranslated(1.2, 0.0, 0.0);				//move to the end of 1st link
		glRotated((double)neck, 0.0, 0.0, 1.0);
		glTranslated(-3.4, 0.0, 0.0);
		glPushMatrix();
			glScaled(0.5, 0.9, 0.9);
			glutSolidCube(1.0);
		glPopMatrix();
	// 2nd link
		glTranslated(1.2, 0.0, 0.0);				//move to the end of 2ndlink
		glRotated((double)head-10, 0.0, 0.0, 1.0);
		glTranslated(-2.0, 0.0, 0.0);
		glPushMatrix();
			glScaled(1.2, 1.5, 1.5);
			glutSolidCube(1.0);
		glPopMatrix();
	glPopMatrix();
	

// 左手
	glPushMatrix();
	/* 1st link */
		glTranslated(0.0, 3.0, -1.5);
		glRotated((double)left_shoulder, 0.0, 0.0, 1.0);
		glTranslated(0.2, 0.5, 0.0);
		
		glPushMatrix();
			glScaled(1.5, 0.5, 1.0);
			glutSolidCube(1.0);
		glPopMatrix();
	/* 2nd link */
		glTranslated(1.0, 0.0, 0.0);				//move to the end of 1st link
		glRotated((double)left_elbow, 0.0, -1.0, 1.0);
		glTranslated(0.7, 0.2, 0.0);
		glPushMatrix();
			glScaled(1.3, 0.5, 0.9);
			glutSolidCube(1.0);
		glPopMatrix();
	/* 3rd link */
		glTranslated(0.5, 0.0, 0.0);				//move to the end of 2nd link
		glRotated((double)left_hand, 0.0, 0.0, 1.0);
		glTranslated(0.0, 0.0, 0.0);
		glPushMatrix();
			glScaled(0.4, 0.8, 1.0);
			glutSolidCube(1.0);
		glPopMatrix();
	glPopMatrix();

// 右手(右手の先にピストルをつける)
	glPushMatrix();
	/* 1st link */
		glTranslated(0.0, 3.0, 1.5);
		glRotated((double)right_shoulder, 0.0, 0.0, 1.0);
		glTranslated(0.2, 0.5, 0.0);

		glPushMatrix();
			glScaled(1.5, 0.5, 1.0);
			glutSolidCube(1.0);
		glPopMatrix();
	/* 2nd link */
		glTranslated(0.8, 0.0, 0.0);				//move to the end of 1st link
		glRotated((double)right_elbow, 0.6, 0.8, 0.0); //0.5 0.5 0.0
		glTranslated(0.8, 0.2, 0.0);
		glPushMatrix();
			glScaled(1.3, 0.5, 0.9);
			glutSolidCube(1.0);
		glPopMatrix();
	/* 3rd link */
		glTranslated(0.5, 0.0, 0.0);				//move to the end of 2nd link
		glRotated((double)right_hand, 0.0, 0.0, 1.0);
		glTranslated(0.0, 0.0, 0.0);
		glPushMatrix();
			glScaled(0.4, 0.8, 1.0);
			glutSolidCube(1.0);
		glPopMatrix();
	// ピストルの描画
	// 4th link
		glTranslated(0.5, 0.0, 0.0);				//move to the end of 3rd link
		glRotated((double)pistol_handle, 1.8, 1.0, 3.0);
		glTranslated(0.0, 0.0, 0.0);
		glPushMatrix();
			glScaled(0.6, 1.2, 0.5);
			glutSolidCube(1.0);
		glPopMatrix();
	// 5th link
		glTranslated(0.1, 0.0, 0.0);				//move to the end of 4th link
		glRotated((double)pistol_muzzle, 1.0, 0.0, 0.0);
		glTranslated(0.0, 0.7, 0.6);
			glPushMatrix();
			glScaled(0.6, 1.6, 0.5);
			glutSolidCube(1.0);
		glPopMatrix();
	// 弾丸の描画
		glTranslated(0.0, 0.0, 0.0);				//move to the end of 4th link
		glRotated(25.0, 0.0, 1.0, 1.0);
		glTranslated((double)bullet, 0.0, 0.0);
		glPushMatrix();
			glScaled(0.6, 1.6, 0.5);
			glutSolidSphere(0.2, 20, 20);
		glPopMatrix();
	glPopMatrix();
// まとめて動かす
	glPopMatrix();

// 道
	glPushMatrix();
		glRotated(0.0, 0.0, 0.0, 0.0);
		glTranslated(100.0, -5.0, 2.0);

		glPushMatrix();
		glScaled(250.0, 3.0, 60.0);
		glutSolidCube(1.0);
		glPopMatrix();
	glPopMatrix();

// ガードレール
	/* 1st pole */
	glPushMatrix();
	glRotated(-90.0, 0.0, 1.0, 0.0);
	glTranslated(-15.0, 0.0, -2.0);
		glPushMatrix();
			glRotated(0.0, 0.0, 0.0, 0.0);
			glTranslated((double)guardrail_pos, 0.0, -2.5);

			glPushMatrix();
				glScaled(0.5, 3.5, 0.5);
				glutSolidCube(1.0);
			glPopMatrix();
		glPopMatrix();

		/* 2nd pole */
		glPushMatrix();
			glRotated(0.0, 0.0, 0.0, 0.0);
			glTranslated((double)guardrail_pos+14.0, 0.0, -2.5);

			glPushMatrix();
				glScaled(0.5, 3.5, 0.5);
				glutSolidCube(1.0);
			glPopMatrix();
		glPopMatrix();

		/* 3rd board */
		glPushMatrix();
			glRotated(0.0, 0.0, 0.0, 0.0);
			glTranslated((double)guardrail_pos+7.0, 1.0, -2.5);

			glPushMatrix();
				glScaled(16.0, 1.0, 0.2);
				glutSolidCube(1.0);
			glPopMatrix();
		glPopMatrix();
	glPopMatrix();

	glutSwapBuffers();
	glFlush();
}

void myReshape (int width, int height)
{
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, (double)width/(double)height, 0.1, 50.0); // クリッピング面
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslated(-2.0, -2.0, -10.0);				// move to enable viewing
	glRotated(90.0, 0.0, 1.0, 0.0);
	// cam_pos_x, y, z, cam_look_pos_x, y, z, cam_up_x, y, z
	//gluLookAt(2.0, 3.0, 10.0, cam_x+2, cam_y, -10.0, 0.0, 1.0, 0.2);
}

void myKeyboard (unsigned char key, int x, int y)
{
	switch (key) {
		// 前進
		case 'w':
			robot_x += 0.1;
			//robot_x += sin(robot_rotate) * 0.1;
			//robot_z += cos(robot_rotate) * 0.1;
			//r = hypot(robot_x, robot_z) + 0.1;
			//robot_x += (robot_x * cos(robot_rotate) - robot_z * sin(robot_rotate)) * 0.01;
			//robot_z += (robot_x * sin(robot_rotate) + robot_z * cos(robot_rotate)) * 0.01;
			//robot_z += r * cos(robot_rotate);
			//robot_x += r * sin(robot_rotate);
			// 右足・左足を動かす
			right_knee = (right_knee + direction) % 360;
			left_knee = (left_knee + direction * -1) % 360;
			// つま先を微調整
			left_toe = (left_toe + direction * -1) % 360;
			right_toe = (right_toe + direction) % 360;

			if (left_knee <= -110) {
				direction *= -1;
			}
			else if (left_knee >= -70) {
				direction *= -1;
			}

			if (robot_x >= 7.0) {
				robot_x = 7.0;
			}
			glutPostRedisplay();
			break;
		// 前進
		case 'W':
			robot_x += 0.1;

			// 右足・左足を動かす
			right_knee = (right_knee + direction) % 360;
			left_knee = (left_knee + direction * -1) % 360;
			// つま先を微調整
			left_toe = (left_toe + direction * -1) % 360;
			right_toe = (right_toe + direction) % 360;

			if (left_knee <= -110) {
				direction *= -1;
			}
			else if (left_knee >= -70) {
				direction *= -1;
			}

			if (robot_x >= 7.0) {
				robot_x = 7.0;
			}
			glutPostRedisplay();
			break;
		// 後退
		case 's':
			robot_x -= 0.1;
			//robot_x -= cos(robot_rotate);
			//robot_z -= sin(robot_rotate);
			// 右足・左足を動かす
			right_knee = (right_knee + direction) % 360;
			left_knee = (left_knee + direction * -1) % 360;
			// つま先を微調整
			left_toe = (left_toe + direction * -1) % 360;
			right_toe = (right_toe + direction) % 360;

			if (left_knee <= -110) {
				direction *= -1;
			}
			else if (left_knee >= -70) {
				direction *= -1;
			}
			if (robot_x <= 3.0) {
				robot_x = 3.0;
			}
			glutPostRedisplay();
			break;
		// 後退
		case 'S':
			robot_x -= 0.1;
			//robot_x -= cos(robot_rotate);
			//robot_z -= sin(robot_rotate);
			// 右足・左足を動かす
			right_knee = (right_knee + direction) % 360;
			left_knee = (left_knee + direction * -1) % 360;
			// つま先を微調整
			left_toe = (left_toe + direction * -1) % 360;
			right_toe = (right_toe + direction) % 360;

			if (left_knee <= -110) {
				direction *= -1;
			}
			else if (left_knee >= -70) {
				direction *= -1;
			}
			if (robot_x <= 3.0) {
				robot_x = 3.0;
			}
			glutPostRedisplay();
			break;
		// 右移動
		case 'd':
			robot_z += 0.1;
			// 右足・左足を動かす
			right_knee = (right_knee + direction) % 360;
			left_knee = (left_knee + direction * -1) % 360;
			// つま先を微調整
			left_toe = (left_toe + direction * -1) % 360;
			right_toe = (right_toe + direction) % 360;

			if (left_knee <= -110) {
				direction *= -1;
			}
			else if (left_knee >= -70) {
				direction *= -1;
			}
			if (robot_x <= 3.0) {
				robot_x = 3.0;
			}
			glutPostRedisplay();
			if(robot_z >= 11.0) {
				robot_z = 11.0;
			}
			break;
		// 右移動
		case 'D':
			robot_z += 0.1;
			// 右足・左足を動かす
			right_knee = (right_knee + direction) % 360;
			left_knee = (left_knee + direction * -1) % 360;
			// つま先を微調整
			left_toe = (left_toe + direction * -1) % 360;
			right_toe = (right_toe + direction) % 360;

			if (left_knee <= -110) {
				direction *= -1;
			}
			else if (left_knee >= -70) {
				direction *= -1;
			}
			if (robot_x <= 3.0) {
				robot_x = 3.0;
			}
			glutPostRedisplay();
			if (robot_z >= 11.0) {
				robot_z = 11.0;
			}
			break;
		// 左移動
		case 'a':
			robot_z -= 0.1;
			// 右足・左足を動かす
			right_knee = (right_knee + direction) % 360;
			left_knee = (left_knee + direction * -1) % 360;
			// つま先を微調整
			left_toe = (left_toe + direction * -1) % 360;
			right_toe = (right_toe + direction) % 360;

			if (left_knee <= -110) {
				direction *= -1;
			}
			else if (left_knee >= -70) {
				direction *= -1;
			}
			if (robot_x <= 3.0) {
				robot_x = 3.0;
			}
			glutPostRedisplay();
			if (robot_z <= 2.5) {
				robot_z = 2.5;
			}
			break;
		// 左移動
		case 'A':
			robot_z -= 0.1;
			// 右足・左足を動かす
			right_knee = (right_knee + direction) % 360;
			left_knee = (left_knee + direction * -1) % 360;
			// つま先を微調整
			left_toe = (left_toe + direction * -1) % 360;
			right_toe = (right_toe + direction) % 360;

			if (left_knee <= -110) {
				direction *= -1;
			}
			else if (left_knee >= -70) {
				direction *= -1;
			}
			if (robot_x <= 3.0) {
				robot_x = 3.0;
			}
			glutPostRedisplay();
			if (robot_z <= 2.5) {
				robot_z = 2.5;
			}
			break;
		// デバッグ用()
		/*
		case 'e':
			score++;
			target_pos = rand() % 10 - 3;
			break;
		*/
		// 発砲
		case ' ':
			bullet_flag = 1;
			break;
		case 27:
			exit(0);
			break;
		default:
			break;
	}
}

// 時間経過処理
void myTimer(int value)
{
	if (value == 1)
	{
		glutTimerFunc(samplingTime, myTimer, 1);

		// タイマーカウント
		timer_flag--;
		if (timer_flag == 0) {
			if (count_down > 0) {
				count_down--;
			}
			// タイマーが0になったときに最終スコアにスコアを代入する
			else if (count_down == 0) {
				if (final_flag) {
					final_score = score;
					final_flag = 0;
				}
			}
			timer_flag = 20;
		}

		// 時間経過でターゲット移動(5秒おき)
		target_timer--;
		if (target_timer == 0) {
			target_pos = rand() % 10 - 3;
			target_timer = 100;
		}

		// 弾丸の移動
		if (bullet_flag) {
			bullet += 2;
			if (bullet >= 50) {
				// 当たり判定
				if (robot_z -0.5 <= target_pos && target_pos <= robot_z + 0.5) {
					target_pos = rand() % 10 - 3;
					score += 3;
				} else if (robot_z -2 <= target_pos && target_pos <= robot_z + 2) {
					target_pos = rand() % 10 - 3;
					score++;
				}
				bullet = 0;
				bullet_flag = 0;
			}
		}

		glutPostRedisplay();
	}
}

// マウスドラッグによる背景色と光源変更
void myMotion(int x, int y){
	// 初期位置の設定(1回のみ実行)
	if (we) {
		first_x = x;
		first_y = y;
		we = 0;
	}
	sub_x = first_x - x;
	sub_y = first_y - y;

	// カメラ位置の指定
	cam_x = sub_x;
	cam_y = sub_y;

	// マウスドラッグによりロボット回転
	robot_rotate = (int)(robot_rotate + (sub_x * 0.01)) % 360;

	bg_color = robot_rotate;
	//bg_color = sub_x / 360 + 0.6;

	// 度数に応じて色変更(朝・昼・夕・夜)
	if (bg_color >= 0 && bg_color < 90) {
		// 朝
		glClearColor(0.6314, 0.8118, 0.9569, 1.0);
		// 光源の位置や光源の色設定
		light_x = -2;
		light_y = -1;
		light_z = 1;
		light_r = 0.6314;
		light_g = 0.8118;
		light_b = 0.9569;
		light_0_x = -5.0;
	} else if(bg_color >= 90 && bg_color < 180) {
		// 昼
		glClearColor(0.0000, 0.4039, 0.7529, 1.0);
		// 光源の位置や光源の色設定
		light_x = 0;
		light_y = -5;
		light_z = 1;
		light_r = 0.0000;
		light_g = 0.4039;
		light_b = 0.7529;
		light_0_x = -5.0;
	} else if (bg_color >= 180 && bg_color < 270) {
		// 夕
		glClearColor(0.9922, 0.4941, 0.0000, 1.0);
		// 光源の位置や光源の色設定
		light_x = 10;
		light_y = -1;
		light_z = 1;
		light_r = 0.9922;
		light_g = 0.4941;
		light_b = 0.0000;
		light_0_x = -5.0;
	} else if (bg_color >= 270 && bg_color <= 360) {
		// 夜
		glClearColor(0.1333, 0.2275, 0.4392, 1.0);
		// 光源の位置や光源の色設定
		light_x = 0;
		light_y = 0;
		light_z = 1;
		light_r = 0.1333;
		light_g = 0.2275;
		light_b = 0.4392;
		light_0_x = 15.0;
	} else if (bg_color < 0 && bg_color > -90) {
		// 朝
		glClearColor(0.6314, 0.8118, 0.9569, 1.0);
		// 光源の位置や光源の色設定
		light_x = -2;
		light_y = -1;
		light_z = 1;
		light_r = 0.6314;
		light_g = 0.8118;
		light_b = 0.9569;
		light_0_x = -5.0;
	} else if (bg_color <= -90 && bg_color > -180) {
		// 昼
		glClearColor(0.0000, 0.4039, 0.7529, 1.0);
		// 光源の位置や光源の色設定
		light_x = 0;
		light_y = -5;
		light_z = 1;
		light_r = 0.0000;
		light_g = 0.4039;
		light_b = 0.7529;
		light_0_x = -5.0;
	} else if (bg_color <= -180 && bg_color > -270) {
		// 夕
		glClearColor(0.9922, 0.4941, 0.0000, 1.0);
		// 光源の位置や光源の色設定
		light_x = 10;
		light_y = -1;
		light_z = 1;
		light_r = 0.9922;
		light_g = 0.4941;
		light_b = 0.0000;
		light_0_x = -5.0;
	} else {
		// 夜
		glClearColor(0.1333, 0.2275, 0.4392, 1.0);
		// 光源の位置や光源の色設定
		light_x = 0;
		light_y = 0;
		light_z = 1;
		light_r = 0.1333;
		light_g = 0.2275;
		light_b = 0.4392;
		light_0_x = 15.0;
	}
	mySetLight();
}

// Idle状態は常に再描画し続ける
void myIdle(void) {
	glutPostRedisplay();
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	myInit(argv[0]);
	setUpTexture();
	mySetLight();
	mySetMenu();
	glutKeyboardFunc(myKeyboard);
	glutMotionFunc(myMotion);
	glutIdleFunc(myIdle);
	glutTimerFunc(samplingTime, myTimer, 1);
	glutReshapeFunc(myReshape);
	glutDisplayFunc(myDisplay);
	glutMainLoop();
	return 0;
}