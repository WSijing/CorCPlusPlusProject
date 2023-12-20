/*开发日志
* 1.实现初始界面
* 2.实现植物卡槽
* 3.实现植物的选取和拖动
* 4.实现植物的种植
* 5.实现植物的摇摆
* 6.实现菜单界面
* 7.实现随机阳光掉落
* 8.实现收集阳光和显示阳光值
* 9.创建僵尸和实现其行走
* 10.实现阳光球飞到阳光计数器的过程
* 11.实现豌豆子弹的发射和渲染
* 12.实现豌豆子弹与僵尸的碰撞检测
* 13.实现僵尸死亡
* 14.实现僵尸吃植物
* 15.实现向日葵生成阳光
* 16.片头巡场
* 17.判断游戏结束
* 18.增加樱桃炸弹
*/

#include <stdio.h>
#include "tools.h"
#include <graphics.h>
#include <math.h>
#include <time.h>
#include <math.h>
#include <mmsystem.h>
#include "vector2.h"

#pragma comment(lib, "winmm.lib")

#define WIN_WIDTH 900
#define WIN_HEIGHT 600
#define ZOMBIE_MAX 10

enum {WAN_DOU, XIANG_RI_KUI, YING_TAO_ZHA_DAN, ZHI_WU_COUNT};
enum {SUNSHINE_DOWN, SUNSHINE_GROUND, SUNSHINE_COLLECT, SUNSHINE_PRODUCT};
enum {GOING, WIN, FAIL};

struct averageZhiwu {
	int type; //植物类型 0：无 1：第一种植物
	int frameIndex; //帧序号
	int deadTime; //死亡计时器

	int timer;
	int x, y;
	
	int shootTime;
} map[5][9] = { 0 };

struct visualZhiwu {
	int type; //植物类型 0：无 1：第一种植物
	int x, y; //鼠标指向的位置
} unmap[5][9] = { 0 }, checkUnmap = { 0 };

struct sunshineBall {
	int x, y; //飘落过程中的位置
	int frameIndex; //帧序号
	int destY; //落点位置
	bool used; //是否使用过，1：未使用过 0：使用过
	int timer;

	float t; //贝塞尔时间点 0..1
	vector2 p1, p2, p3, p4;
	vector2 currentPresentation;
	float speed;
	int status;
} balls[20] = { 0 };

struct zombie {
	int x, y;
	int frameIndex;
	bool used;
	double speed;
	int row;
	int blood; //僵尸血量
	bool dead;  //一般死亡
	bool eating;
	bool boomDead; //因灰烬植物而死亡
} zombies[10] = { 0 };

struct bullet {
	int x, y;
	bool used;
	int speed;
	int row;
	bool blast;
	int frameIndex;
} bullets[30] = { 0 };

IMAGE imgBackground = 0; //背景图片
IMAGE imgBar5 = 0; //植物卡槽
IMAGE imgBulletNormal = 0;
IMAGE imgBulletBlast[4] = { 0 };
IMAGE imgCards[ZHI_WU_COUNT] = { 0 }; //植物卡牌
IMAGE imgZombieDead[10] = { 0 };
IMAGE imgZombieBoomDead[20] = { 0 };
IMAGE imgZombieStand[11] = { 0 };
IMAGE imgZombieEating[21] = { 0 };
IMAGE imgZombie[22] = { 0 };
IMAGE imgSunshineBall[29] = { 0 };
IMAGE* imgZhiWu[ZHI_WU_COUNT][20] = { 0 }; //植物帧图

int curX = 0, curY = 0; //当前选中植物在移动过程中的位置
int curZhiWu = 0; //0：未选中 1：选择第一种植物
bool firstDown = 0; // 判断抓取还是放下
int sunshine = 50; //阳光值
int killCount = 0; //已击杀僵尸数
int zombieCount = 0; //已出现僵尸数
int gameStatus = GOING; //游戏状态

bool fileExist(const char* name) {
	FILE* fp = fopen(name, "r");
	if (fp) {
		fclose(fp);
		return 1;
	}
	else {
		return 0;
	}
}

void gameInit() {
	//加载背景图片
	loadimage(&imgBackground, "res/bg.jpg");
	loadimage(&imgBar5, "res/bar5.png");

	//初始化植物相关状态
	memset(imgZhiWu, 0, sizeof(imgZhiWu));
	memset(map, 0, sizeof(map));
	memset(unmap, 0, sizeof(unmap));

	killCount = 0;
	zombieCount = 0;
	gameStatus = GOING;

	//创建植物卡牌
	char name[64];

	for (int i = 0; i < ZHI_WU_COUNT; i++) {
		//生成植物卡牌的文件名
		sprintf_s(name, sizeof(name), "res/Cards/card_%d.png", i + 1);
		loadimage(&imgCards[i], name);

		//生成植物摇摆帧图的文件名
		for (int j = 0; j < 20; j++) {
			sprintf_s(name, sizeof(name), "res/zhiwu/%d/%d.png", i, j);

			//判断文件的存在性
			if (fileExist(name)) {
				imgZhiWu[i][j] = new IMAGE;
				loadimage(imgZhiWu[i][j], name);
			}
			else {
				break;
			}
		}
	}

	curZhiWu = 0; //初始化选中植物状态
	sunshine = 50; //初始化阳光值

	
	memset(balls, 0, sizeof(balls));
	for (int i = 0; i < 29; i++) {
		sprintf_s(name, sizeof(name), "res/sunshine/%d.png", i + 1);
		loadimage(imgSunshineBall + i, name);
	}


	//随机种子
	srand(time(NULL));

	//创建游戏图形窗口
	initgraph(WIN_WIDTH, WIN_HEIGHT, 1);

	//设置字体
	LOGFONT font;
	getfont(&font);
	font.lfHeight = 30;
	font.lfWeight = 15;
	strcpy(font.lfFaceName, "Segoe UI Black");
	font.lfQuality = ANTIALIASED_QUALITY; //抗锯齿质量
	settextstyle(&font);
	setbkmode(TRANSPARENT);
	setcolor(BLACK);

	//初始化僵尸数据
	memset(zombies, 0, sizeof(zombies));
	for (int i = 0; i < 22; i++) {
		sprintf_s(name, sizeof(name), "res/zm/%d.png", i + 1);
		loadimage(&imgZombie[i], name);
	}

	//初始化豌豆子弹的帧图片数组
	loadimage(&imgBulletNormal, "res/bullets/bullet_normal.png");
	memset(bullets, 0, sizeof(bullets));

	//初始化豌豆子弹的破裂帧图片数组
	loadimage(&imgBulletBlast[3], "res/bullets/bullet_blast.png");
	for (int i = 0; i < 3; i++) {
		float k = (i + 1) * 0.2;
		loadimage(&imgBulletBlast[i], "res/bullets/bullet_blast.png", imgBulletBlast[3].getwidth() * k, imgBulletBlast[3].getheight() * k,1);
	}

	//初始化僵尸吃植物的帧图片数组
	for (int i = 0; i < 21; i++) {
		sprintf_s(name, sizeof(name), "res/zm_eat/%d.png", i + 1);
		loadimage(&imgZombieEating[i], name);
	}

	//初始化僵尸死亡的帧图片数组
	for (int i = 0; i < 11; i++) {
		sprintf_s(name, sizeof(name), "res/zm_dead/%d.png", i + 1);
		loadimage(&imgZombieDead[i], name);
	}

	//初始化植物因灰烬植物死亡的帧图片数组
	for (int i = 0; i < 21; i++) {
		sprintf_s(name, sizeof(name), "res/zm_dead2/%d.png", i + 1);
		loadimage(&imgZombieBoomDead[i], name);
	}
	//初始化开场僵尸的帧图片数组
	for (int i = 0; i < 11; i++) {
		sprintf_s(name, sizeof(name), "res/zm_stand/%d.png", i + 1);
		loadimage(&imgZombieStand[i], name);
	}
}

void drawCards() {
	//植物卡牌填充卡槽
	for (int i = 0; i < ZHI_WU_COUNT; i++) {
		int x = 268 - 112 + i * 65;
		int y = 6;
		putimage(x, y, &imgCards[i]);
	}
}

void drawPlant() {
	//一帧一帧输出，实现植物摇摆
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type > 0) {
				int zhiWuType = map[i][j].type - 1;
				int index = map[i][j].frameIndex;
				if (!index) {
					index++;
				}
				if(map[i][j].type == YING_TAO_ZHA_DAN + 1 && map[i][j].frameIndex >= 8)
					putimagePNG(map[i][j].x - 70, map[i][j].y + 15 - 70, imgZhiWu[zhiWuType][index]);
				else
					putimagePNG(map[i][j].x, map[i][j].y + 15, imgZhiWu[zhiWuType][index]);
			}
			if (map[i][j].type == YING_TAO_ZHA_DAN + 1 && map[i][j].frameIndex >= 19) {
				map[i][j].type = 0;
			}
		}
	}

	//渲染拖动过程的植物
	if (curZhiWu) {
		bool changeFlag = 1;
		for (int i = 0; i < 5; i++) {
			for (int j = 0; j < 9; j++) {
				if (unmap[i][j].type > 0 && map[i][j].type == 0) {
					unmap[i][j].x = 259 - 112 + j * 81;
					unmap[i][j].y = 77 + 15 + i * 102;
					IMAGE* unimg = imgZhiWu[unmap[i][j].type - 1][0];
					putimagePNG(unmap[i][j].x, unmap[i][j].y, unimg);
					if (unmap[i][j].x == checkUnmap.x && unmap[i][j].y == checkUnmap.y) {
						changeFlag = 0;
					}
					checkUnmap = unmap[i][j];
				}
			}
		}
		if (changeFlag) {
			memset(unmap, 0, sizeof(unmap));
		}
		IMAGE* img = imgZhiWu[curZhiWu - 1][1];
		putimagePNG(curX - img->getwidth() / 2, curY - img->getheight() / 2, img);
	}
	else {
		memset(unmap, 0, sizeof(unmap));
	}
}

void drawSunshine() {
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < ballMax; i++) {
		if(balls[i].used || balls[i].status == SUNSHINE_COLLECT) {
			IMAGE* img = &imgSunshineBall[balls[i].frameIndex];
			putimagePNG(balls[i].currentPresentation.x, balls[i].currentPresentation.y, img);
		}
	}

	//显示阳光值
	char scoreText[8];
	sprintf_s(scoreText, sizeof(scoreText), "%d", sunshine);
	int textX = 220 - 112, tmp = sunshine;
	while (tmp) {
		textX -= 5;
		tmp /= 10;
	}
	outtextxy(textX, 67, scoreText);
}

void drawZombie() {
	int zombieMax = sizeof(zombies) / sizeof(zombies[0]);
	for (int i = 0; i < zombieMax; i++) {
		if (zombies[i].used) {
			IMAGE* img = NULL;
			if (zombies[i].dead) {
				img = imgZombieDead;
			}
			else if (zombies[i].boomDead) {
				img = imgZombieBoomDead;
			}
			else if (zombies[i].eating) {
				img = imgZombieEating;
			}
			else {
				img = imgZombie;
			}
			img += zombies[i].frameIndex;
			// 用于“玩玩小游戏”
			//for (int j = 0; j < 10; j++) {
			//	if (img == &imgZombieDead[j]) {
			//		putimagePNG(zombies[i].x, zombies[i].y - 144, img);
			//	}
			//}
			putimagePNG(zombies[i].x, zombies[i].y - 144, img);
		}
	}
}

void drawBullet() {
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
	for (int i = 0; i < bulletMax; i++) {
		if (bullets[i].used) {
			if (bullets[i].blast) {
				IMAGE* img = &imgBulletBlast[bullets[i].frameIndex];
				putimagePNG(bullets[i].x, bullets[i].y, img);
			}
			else {
				putimagePNG(bullets[i].x, bullets[i].y, &imgBulletNormal);
			}
		}
	}
}

void updateWindow() {
	BeginBatchDraw(); //开始缓冲
	
	//显示游戏背景和植物卡槽
	putimage(-112, 15, &imgBackground);
	putimagePNG(180 - 112, 0, &imgBar5);

	drawCards();
	drawPlant();
	drawZombie();
	drawBullet();
	drawSunshine();

	EndBatchDraw(); //结束缓冲
}

void updatePlant() {
	static int count = 0;
	if (++count < 3) return;
	count = 0;

	//更新摇摆帧
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type > 0) {
				map[i][j].frameIndex++;
				int zhiWuType = map[i][j].type - 1;
				int index = map[i][j].frameIndex;
				if (imgZhiWu[zhiWuType][index] == NULL) {
					map[i][j].frameIndex = 0;
				}
			}
		}
	}
}

void createSunshine() {
	static int count;
	count++;
	static int fre = 110;
	if (count >= fre) {
		fre = 500 + rand() % 75;
		count = 0;

		//从阳光池中取一个可用阳光
		int ballMax = sizeof(balls) / sizeof(balls[0]);
		int i = 0;
		while (i < ballMax && balls[i].used) i++;
		if (i >= ballMax) return;
		
		balls[i].used = 1;
		balls[i].frameIndex = 0;
		balls[i].timer = 0;
		balls[i].status = SUNSHINE_DOWN;
		balls[i].t = 0;
		balls[i].p1 = vector2(260 - 112 + rand() % (900 - 260), 60);
		balls[i].p4 = vector2(balls[i].p1.x, 200 + (rand() % 4) * 90);
		float off = 2.0;
		float distance = balls[i].p4.y - balls[i].p1.y;
		balls[i].speed = 1.5 / (distance / off);
	}

	//向日葵生产阳光
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type == XIANG_RI_KUI + 1) {
				map[i][j].timer++;

				if (map[i][j].timer > 500) {
					map[i][j].timer = 0;

					int k;
					for (k = 0; k < ballMax && balls[k].used; k++) {
						if (k >= ballMax) return;
					}
					balls[k].used = 1;
					balls[k].p1 = vector2(map[i][j].x, map[i][j].y);
					int w = (30 + rand() % 15) * (rand() % 2 ? 1 : -1);
					balls[k].p4 = vector2(map[i][j].x + w,
						map[i][j].y + imgZhiWu[XIANG_RI_KUI][0]->getwidth() - imgSunshineBall[0].getwidth());
					balls[k].p2 = vector2(balls[k].p1.x + w * 0.3, balls[k].p1.y - 100);
					balls[k].p3 = vector2(balls[k].p1.x + w * 0.7, balls[k].p1.y - 100);
					balls[k].status = SUNSHINE_PRODUCT;
					balls[k].speed = 0.05;
					balls[k].t = 0;
				}
			}
		}
	}
}

void updateSunshine() {
	int ballMax = sizeof(balls) / sizeof(balls[0]);

	for (int i = 0; i < ballMax; i++) {
		if (balls[i].used) {
			balls[i].frameIndex = (balls[i].frameIndex + 1) % 29;
			if (balls[i].status == SUNSHINE_DOWN) {
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->currentPresentation = sun->p1 + sun->t * (sun->p4 - sun->p1);
				if (sun->t >= 1) {
					sun->status = SUNSHINE_GROUND;
					sun->timer = 0;
				}
			}
			else if (balls[i].status == SUNSHINE_GROUND) {
				balls[i].timer++;
				if (balls[i].timer > 200) {
					balls[i].timer = 0;
					balls[i].used = 0;
				}
			}
			else if (balls[i].status == SUNSHINE_PRODUCT) {
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->currentPresentation = calcBezierPoint(sun->t, sun->p1, sun->p2, sun->p3, sun->p4);
				if (sun->t > 1) {
					sun->status = SUNSHINE_GROUND;
					sun->timer = 0;
				}
			}
		}
		else if (balls[i].status == SUNSHINE_COLLECT) {
			//配置音乐
			mciSendString("play res/sunshine.mp3", 0, 0, 0);

			struct sunshineBall* sun = &balls[i];
			sun->t += sun->speed;
			sun->currentPresentation = sun->p1 + sun->t * (sun->p4 - sun->p1);
			if (sun->t > 1) {
				sun->used = 0;
				sunshine += 25;
				balls[i].status = -1;
			}
		}
	}
}

void createZombie() {
	if (zombieCount >= ZOMBIE_MAX) {
		return;
	}

	static int zombieFrequence = 300;
	static int count = 0;
	count++;
	if (count > zombieFrequence) {
		count = 0;
		zombieFrequence = 500 + rand() % 200;

		int i;
		int zombieMax = sizeof(zombies) / sizeof(zombies[0]);
		for (i = 0; i < zombieMax && zombies[i].used; i++);
		if (i < zombieMax) {
			memset(&zombies[i], 0, sizeof(zombies[i]));
			zombies[i].used = 1;
			zombies[i].x = WIN_WIDTH;
			zombies[i].row = rand() % 5;
			zombies[i].y = 77 + (1 + zombies[i].row) * 102;
			zombies[i].speed = (100 + rand() % 50) / 100;
			zombies[i].blood = 100;
			zombies[i].dead = 0;
			zombieCount++;
		}
	}
}

void updateZombie() {
	int zombieMax = sizeof(zombies) / sizeof(zombies[0]);

	static int count1 = 0;
	count1++;
	if (count1 > 2) {
		count1 = 0;
		//更新僵尸位置
		for (int i = 0; i < zombieMax; i++) {
			if (zombies[i].used && zombies[i].dead == 0 && zombies[i].boomDead == 0) {
				zombies[i].x -= zombies[i].speed;
				if (zombies[i].x < 56) {
					gameStatus = FAIL;
				}
			}
		}
	}

	static int count2 = 0;
	count2++;
	if (count2 > 2) {
		count2 = 0;
		for (int i = 0; i < zombieMax; i++) {
			if (zombies[i].used) {
				if (zombies[i].dead || zombies[i].boomDead) {
					zombies[i].frameIndex++;
					//如果僵尸死亡就增加死亡的帧数，并在加完之后删除僵尸
					if ((zombies[i].dead == 1 && zombies[i].frameIndex > 11) || (zombies[i].boomDead == 1 && zombies[i].frameIndex > 21)) {
						zombies[i].used = 0;
						killCount++;
						if (killCount == ZOMBIE_MAX) {
							gameStatus = WIN;
						}
					}
				}
				else if (zombies[i].eating) {
					zombies[i].frameIndex = (zombies[i].frameIndex + 1) % 21;
				}
				else {
					zombies[i].frameIndex = (zombies[i].frameIndex + 1) % 22;
				}
			}
		}
	}
	
}

void shoot() {
	static int count1 = 0;
	if (++count1 < 3) return;
	count1 = 0;

	int lines[5] = { 0 };
	int zombieMax = sizeof(zombies) / sizeof(zombies[0]);
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
	int dangerX = WIN_WIDTH - 30;

	// 判断僵尸所在的行
	for (int i = 0; i < zombieMax; i++) {
		if (zombies[i].used && zombies[i].x < dangerX) {
			lines[zombies[i].row] = 1;
		}
	}

	for (int i = 0; i < 5; i++) { 
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type == WAN_DOU + 1 && lines[i]) {
				for (int k = 0; k < zombieMax; k++) {
					if (zombies[k].row == i && map[i][j].x < zombies[k].x) {
						map[i][j].shootTime++;
						if (map[i][j].shootTime > 20) {
							map[i][j].shootTime = 0;
							int k = 0;
						
							for (k = 0; k < bulletMax && bullets[k].used; k++);
							if (k < bulletMax) {
								bullets[k].used = 1;
								bullets[k].row = i;
								bullets[k].speed = 24;

								bullets[k].blast = 0;
								bullets[k].frameIndex = 0;
								int zhiwuX = 256 - 112 + j * 81;
								int zhiwuY = 77 + i * 102 + 27;
								bullets[k].x = zhiwuX + imgZhiWu[map[i][j].type - 1][0]->getwidth() - 10;
								bullets[k].y = zhiwuY;

							}
						}
						break;
					}
				}
				
			}
		}
	}
}

void updateBullets() {
	static int count = 0;
	if (++count < 2) return;
	count = 0;

	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
	for (int i = 0; i < bulletMax; i++) {
		if (bullets[i].used) {
			bullets[i].x += bullets[i].speed;
			if (bullets[i].x > WIN_WIDTH) {
				bullets[i].used = 0;
			}

			if (bullets[i].blast) {
				bullets[i].frameIndex++;
				if (bullets[i].frameIndex > 3) {
					bullets[i].used = 0;
				}
			}
		}
	}
}

void bullet2Zombie() {
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
	int zombieMax = sizeof(zombies) / sizeof(zombies[0]);
	for (int i = 0; i < bulletMax; i++) {
		if (bullets[i].used == 0 || bullets[i].blast) continue;

		for (int j = 0; j < zombieMax; j++) {
			if (zombies[j].used == 0) continue;
			int x1 = zombies[j].x + 80;
			int x2 = zombies[j].x + 110;
			int x = bullets[i].x;
			if (bullets[i].row == zombies[j].row && x > x1 && x < x2 && zombies[j].dead == 0) {
				zombies[j].blood -= 10;
				bullets[i].blast = 1;
				bullets[i].speed = 0;

				if (zombies[j].blood <= 0) {
					zombies[j].dead = 1;
					zombies[j].speed = 0;
					zombies[j].frameIndex = 0;
				}
			}
		}
	}
}

void zombie2Plant() {
	int zombieMax = sizeof(zombies) / sizeof(zombies[0]);
	for (int i = 0; i < zombieMax; i++) {

		int row = zombies[i].row;

		for (int j = 0; j < 9; j++) {
			if (map[row][j].type == 0) continue;
			int plantX = 256 - 112 + j * 81;
			int x1 = plantX + 10;
			int x2 = plantX + 60;
			int x3 = zombies[i].x + 80;
			if (x3 > x1 && x3 < x2) {
				zombies[i].eating = 1;
				zombies[i].speed = 0;
				for (int k = 0; k < zombieMax; k++) {
					//植物被啃食时进行死亡倒计时，灰烬植物默认无敌
					if (zombies[k].eating && map[row][j].type != YING_TAO_ZHA_DAN + 1) {
						map[row][j].deadTime++;
					}
					if (map[row][j].deadTime > 100) {
						map[row][j].deadTime = 0;
						map[row][j].type = 0;
						zombies[i].eating = 0;
						zombies[i].speed = (100 + rand() % 50) / 100;
					}
				}
			}
		}
	}
}

void collisionCheck() {
	bullet2Zombie();
	zombie2Plant();
}

void updateNearbyZombies(int boomX, int boomY) {
	int zombieMax = sizeof(zombies) / sizeof(zombies[0]);
	for (int i = 0; i < zombieMax; i++) {
		if (zombies[i].used) {
			if (abs(zombies[i].x - boomX) <= 100 && abs(zombies[i].y - boomY) <= 110) {
				zombies[i].boomDead = 1;
			}
		}
	}
}

void boom() {
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type == YING_TAO_ZHA_DAN + 1 && map[i][j].frameIndex == 8) {
				updateNearbyZombies(map[i][j].x - 60, map[i][j].y + 102);
			}
		}
	}
}

void updateGame() {
	updatePlant();

	createSunshine(); //创建阳光
	updateSunshine(); //更新阳光的状态

	createZombie(); //创建僵尸
	updateZombie(); //更新僵尸的状态

	shoot(); //发射豌豆子弹
	updateBullets(); //更新豌豆子弹
	collisionCheck(); //碰撞检测

	boom(); //樱桃炸弹爆炸
}

void collectSunshine(ExMessage* msg) {
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	int width = imgSunshineBall[0].getwidth();
	int height = imgSunshineBall[0].getheight();

	for (int i = 0; i < 10; i++) {
		if (balls[i].used) {
			int x = balls[i].currentPresentation.x;
			int y = balls[i].currentPresentation.y;
			if (msg->x > x && msg->x < x + width && msg->y > y && msg->y < y + height) {
				balls[i].used = 0;
				balls[i].status = SUNSHINE_COLLECT;
				balls[i].p1 = balls[i].currentPresentation;
				balls[i].p4 = vector2(262 - 112 - 70, 0);
				balls[i].t = 0;
				float distance = dis(balls[i].p1 - balls[i].p4);
				float off = 8.0;
				balls[i].speed = 3.0 / (distance / off);


				//设置阳光值最大值
				if (sunshine >= 9999) {
					sunshine = 9999;
				}
				break;
			}
		}
	}
}

void userClick() {
	ExMessage msg;
	static int status = 0;

	if (peekmessage(&msg)) {
		int mmm = 87;
		//实现植物的选取和取消选取
		if (msg.message == WM_LBUTTONDOWN && firstDown == 0) {
			if (msg.x > 268 - 112 && msg.x < 268 - 112 + 65 * ZHI_WU_COUNT && msg.y < 96) {
				int index = (msg.x - 268 + 112) / 65;
				status = 1;
				curZhiWu = index + 1;
				firstDown = 1;
				curX = msg.x;
				curY = msg.y;
			}
			else {
				collectSunshine(&msg);
			}
		}
		else if (msg.message == WM_RBUTTONDOWN && firstDown == 1) {
			firstDown = 0;
			curZhiWu = 0;
			status = 0;
		}
		else if (msg.message == WM_MOUSEMOVE && status == 1) {
			curX = msg.x;
			curY = msg.y;

			if (msg.x > 256 - 112 && msg.x < 985 - 112 && msg.y > 77 + 15 && msg.y < 591 + 15) {
				int row = (msg.y - 77) / 102;
				int col = (msg.x - 256 + 112) / 81;
				unmap[row][col].type = curZhiWu;
			}
		}
		else if (msg.message == WM_LBUTTONDOWN && firstDown == 1) {
			if (msg.x > 256 - 112 && msg.x < 985 - 112 && msg.y > 77 + 15 && msg.y < 591 + 15) {
				int row = (msg.y - 77) / 102;
				int col = (msg.x - 256 + 112) / 81;

				if (map[row][col].type == 0) {
					map[row][col].type = curZhiWu;
					map[row][col].frameIndex = 0;
					map[row][col].shootTime = 0;

					map[row][col].x = 256 - 112 + col * 81;
					map[row][col].y = 77 + row * 102;
				}

				firstDown = 0;
				curZhiWu = 0;
				status = 0;
			}
		}
	}
}

void startUI() {
	IMAGE imgBackground, imgMenu[9];
	char name[64];

	//加载菜单界面各元素图片
	loadimage(&imgBackground, "res/menu.png");
	for (int i = 1; i <= 8; i++) {
		sprintf_s(name, sizeof(name), "res/menu%d.png", i);
		loadimage(imgMenu+i, name);
	}

	bool flag[5] = { 0 };
	while (1) {
		BeginBatchDraw();

		putimage(0, 0, &imgBackground);

		//实现单击高亮和界面跳转
		for (int i = 1; i <= 4; i++) {
			putimagePNG(474, -30 + i * 90, flag[i] ? imgMenu + 2 * i : imgMenu + 2 * i - 1);
		}

		ExMessage msg;
		if (peekmessage(&msg)) {
			if (msg.message == WM_LBUTTONDOWN) {
				for (int i = 0; i <= 4; i++) {
					if (msg.x > 474 && msg.x < 474 + 300 && msg.y > -30 + 90 * i && msg.y < -30 + 90 * (i + 1)) {
						flag[i] = 1;
					}
				}
			}
			else if(msg.message == WM_LBUTTONUP && flag) {
				for (int i = 0; i <= 4; i++) {
					if (msg.x > 474 && msg.x < 474 + 300 && msg.y > -30 + 90 * i && msg.y < -30 + 90 * (i + 1)) {
						return;
					}
					else flag[i] = 0;
				}
			}
		}

		EndBatchDraw();
	}
}

void viewScene() {
	int xMin = WIN_WIDTH - imgBackground.getwidth();
	vector2 points[9] = {
		{550, 80}, {530, 160}, {630, 170}, {530, 200}, {515, 270},
		{565,370}, {605, 340}, {705, 280}, {690, 340} };
	int index[9];
	for (int i = 0; i < 9; i++) {
		index[i] = rand() % 11;
	}
	int count = 0;
	for (int x = 0; x >= xMin; x -= 2) {
		BeginBatchDraw();
		putimage(x, 15, &imgBackground);

		count++;
		for (int k = 0; k < 9; k++) {
			putimagePNG(points[k].x - xMin + x, points[k].y, &imgZombieStand[index[k]]);
			if (count >= 10) {
				index[k] = (index[k] + 1) % 11;
			}
		}
		if (count >= 10)count = 0;
		EndBatchDraw();
		Sleep(5);

		//停留一秒左右
		for (int i = 0; i < 100; i++) {
			BeginBatchDraw();

			putimage(xMin, 15, &imgBackground);
			for (int k = 0; k < 9; k++) {
				putimagePNG(points[k].x, points[k].y, &imgZombieStand[index[k]]);
				index[k] = (index[k] + 1) % 11;
			}
			EndBatchDraw();
			Sleep(30);
		}
		for (int x = xMin; x <= -112; x += 2) {
			BeginBatchDraw();
			putimage(x, 15, &imgBackground);
			count++;
			for (int k = 0; k < 9; k++) {
				putimagePNG(points[k].x - xMin + x, points[k].y, &imgZombieStand[index[k]]);
				if (count >= 10) {
					index[k] = (index[k] + 1) % 11;
				}
				if (count >= 10) count = 0;
			}
			EndBatchDraw();
			Sleep(5);
		}
		break;
	}
}

void barDown() {
	int height = imgBar5.getheight();
	for (int y = -height; y <= 0; y++) {
		BeginBatchDraw();
		putimage(-112, 15, &imgBackground);
		putimagePNG(250 - 112 - 70, y, &imgBar5);

		for (int i = 0; i < ZHI_WU_COUNT; i++) {
			int x = 338 - 112 - 70 + i * 65;

			putimage(x, 6 + y, &imgCards[i]);
		}
		EndBatchDraw();
		Sleep(10);
	}
}

bool checkOver() {
	int ret = false;
	if (gameStatus == WIN) {
		Sleep(2000);
		loadimage(0, "res/win.png");
		ret = true;
	}
	else if (gameStatus == FAIL) {
		Sleep(2000);
		loadimage(0, "res/fail2.png");
		ret = true;
	}
	return ret;
}

int main() {
	gameInit();
	startUI();
	//viewScene();
	//barDown();

	int timer = 0;
	while (1) {
		userClick();
		//延迟输出，减缓更新速度
		timer += getDelay();
		if (timer > 30) {
			timer = 0;
			updateWindow();
			updateGame();
			if (checkOver()) break;
		}
	}

	system("pause");
	return 0;
}