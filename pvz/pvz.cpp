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
* 19.为植物增加阳光消耗和冷却时间
* 20.增加铲子
* 21.增加小推车
* 22.增加坚果墙
* 23.增加土豆雷
* 24.增加音乐音效
* 25.整理代码结构，阶段性完成~~~
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
#define WIN_WIDTH 900 // 屏幕宽度
#define WIN_HEIGHT 600 // 屏幕高度
#define ZOMBIE_MAX 10 // 获胜所需僵尸总数

enum { WAN_DOU, XIANG_RI_KUI, YING_TAO_ZHA_DAN, JIANG_GUO_QIANG, TU_DOU_LEI, ZHI_WU_COUNT }; // 枚举植物种类
enum { SUNSHINE_DOWN, SUNSHINE_GROUND, SUNSHINE_COLLECT, SUNSHINE_PRODUCT }; // 枚举阳光球状态
enum { GOING, WIN, FAIL }; // 枚举游戏进展状态

// 定义草坪上的植物
struct plant {
	bool half; // 坚果墙被啃食剩一半
	bool quarter; // 坚果墙被啃食剩四分之一
	bool ready; // 土豆雷就绪
	bool trigger; // 土豆雷触发
	int type; // 植物类型 0：无；i：第i种植物
	int frameIndex; // 帧序号
	int x, y; // 坐标
	int deadTime; // 死亡计时器
	int timer; // 阳光球制造频率
	int shootTime; // 豌豆射手发射频率
	int readyTime; // 土豆雷就绪所需时间
} map[5][9] = { 0 };

// 定义虚影植物
struct virtualPlant {
	int type; // 植物类型 0：无；i：第i种植物
	int x, y; // 坐标
} unmap[5][9] = { 0 }, checkUnmap = { 0 };

// 定义阳光球
struct sunshineBall {
	bool used; // 是否使用中
	int x, y; // 坐标
	int frameIndex; // 帧序号
	int timer; // 阳光球停留时间，过时消失
	int status; // 阳光球状态
	double t; // 贝塞尔时间点 0...1
	double speed; // 阳光球速度
	vector2 p1, p2, p3, p4; // 四个参数
	vector2 currentPresentation; // 当前位置
} balls[20] = { 0 };

// 定义僵尸
struct zombie {
	bool used; // 是否使用中
	bool eating; // 是否啃食
	bool dead;  // 一般死亡
	bool boomDead; //因灰烬植物死亡
	int x, y; // 坐标
	int frameIndex; // 帧序号
	int row; // 所在行
	int blood; // 血量
	double speed; // 行走速度
} zombies[10] = { 0 };

// 定义豌豆子弹
struct bullet {
	bool used; // 是否使用中
	bool blast; // 是否破裂
	int x, y; // 坐标
	int frameIndex; // 帧序号
	int speed; // 速度
	int row; // 所在行
} bullets[30] = { 0 };

// 定义植物卡牌
struct card {
	bool able;		// 是否可选
	int type;			// 种类
	int cost;			// 购买所需阳光
	int CD;				// 冷却时间
} cards[ZHI_WU_COUNT];

// 定义小推车
struct car {
	bool trigger; // 判断小车是否被触发
	bool exist; // 判断小车是否存在 0：存在 1：不存在
	int x, y; // 坐标
	double speed; // 速度
} cars[5];

IMAGE imgBackground; // 背景图片
IMAGE imgBar5; // 植物卡槽图片
IMAGE imgShovel, imgShovelSlot; // 铲子和铲子槽图片
IMAGE imgCar; // 小推车图片
IMAGE imgUnready; // 土豆雷未就绪图片
IMAGE imgXipu, imgXipuText; // “唏噗”图片
IMAGE imgBulletNormal; // 正常豌豆子弹图片
IMAGE imgBulletBlast[4]; // 豌豆炸裂图片
IMAGE imgCards[ZHI_WU_COUNT], imgCardsBlack[ZHI_WU_COUNT]; // 植物卡牌图片，彩色和黑白
IMAGE imgZombieDead[10]; // 僵尸一般死亡图片
IMAGE imgZombieBoomDead[20]; // 僵尸因灰烬植物死亡图片
IMAGE imgZombieStand[11]; // 巡场时僵尸站立图片
IMAGE imgZombieEating[21]; // 僵尸啃食图片
IMAGE imgZombie[22]; // 僵尸行走图片
IMAGE imgSunshineBall[29]; // 阳光球图片
IMAGE imgWallnutHalf[11], imgWallnutQuarter[15]; // 坚果墙图片
IMAGE* imgPlant[ZHI_WU_COUNT][20]; //植物图片

bool firstDown = 0; // 判断选取还是放下
bool curShovel = 0; // 判断铲子状态
int curX = 0, curY = 0; // 当前选中植物在移动过程中的坐标
int curPlant = 0; // 0：未选中；1：选择第一种植物
int sunshine = 0; // 阳光值
int killCount = 0; // 已击杀僵尸数
int zombieCount = 0; // 已出现僵尸数
int gameStatus = GOING; // 游戏状态

//函数声明
int main(); // 主函数
void gameInit(); // 初始化游戏，加载游戏所需资源
bool fileExist(const char* name); // 判断文件存在性
void startUI(); // 菜单界面
void viewScene(); // 片头巡场
void barDown(); // 卡槽下降
void userClick(); // 对玩家操作的响应
void collectSunshine(ExMessage* msg); // 收集阳光
void updateWindow(); // 游戏画面的实时更新
void drawBullet(); // 渲染豌豆子弹
void drawCar(); // 渲染小推车
void drawCards(); // 渲染植物卡牌
void drawPlant(); // 渲染植物
void drawShovel(); // 渲染铲子
void drawSunshine(); // 渲染阳光值和阳光球
void drawZombie(); // 渲染僵尸
void updateGame(); // 游戏状态的实时更新
void createSunshine(); // 创建阳光球
void createZombie(); // 创建僵尸
void updateBullet(); // 更新豌豆子弹状态
void updateCar(); // 更新小推车状态
void updateNearbyZombies(int type, int boomX, int boomY); // 更新灰烬植物有效范围内的僵尸状态
void updatePlant(); // 更新植物状态
void updateSunshine(); // 更新阳光球状态
void updateZombie(); // 更新僵尸状态
void shoot(); // 判定豌豆发射以及更新豌豆子弹状态
void boom(); // 灰烬植物boom~~~
void collisionCheck(); // 碰撞检测
void potato_mine2Zombie(); // 就绪的土豆雷与僵尸的碰撞检测
void bullet2Zombie(); // 子弹与僵尸的碰撞检测
void car2Zombie(); // 小推车与僵尸的碰撞检测
void zombie2Plant(); // 僵尸与植物的碰撞检测
void judgeWin(); // 判定胜利
bool checkOver(); // 游戏结束判定

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
	char name[64];// 加载背景图片
	loadimage(&imgBackground, "res/bg.jpg");
	loadimage(&imgBar5, "res/bar5.png");
	// 初始化植物相关状态
	memset(imgPlant, 0, sizeof(imgPlant));
	memset(map, 0, sizeof(map));
	memset(unmap, 0, sizeof(unmap));
	memset(cards, 0, sizeof(cards));
	// 初始化游戏状态判定依据
	killCount = 0;
	zombieCount = 0;
	gameStatus = GOING;
	// 创建植物卡牌
	for (int i = 0; i < ZHI_WU_COUNT; i++) {
		// 生成植物卡牌的文件名
		sprintf_s(name, sizeof(name), "res/Cards/card_%d.png", i + 1);   // 加载正常卡牌
		loadimage(&imgCards[i], name);
		sprintf_s(name, sizeof(name), "res/Cards_Black/card_%d.png", i + 1);	// 加载黑白卡牌（阳光不足或没冷却好）
		loadimage(&imgCardsBlack[i], name);
		// 生成植物摇摆帧图的文件名
		for (int j = 0; j < 20; j++) {
			sprintf_s(name, sizeof(name), "res/plants/%d/%d.png", i, j);
			// 判断文件的存在性
			if (fileExist(name)) {
				imgPlant[i][j] = new IMAGE;
				loadimage(imgPlant[i][j], name);
			}
			else {
				break;
			}
		}
	}
	// 植物花费设置
	for (int i = 0; i < ZHI_WU_COUNT; i++) {
		cards[i].type = i;
		if (cards[i].type == WAN_DOU) {
			cards[i].cost = 100;
		}
		else if (cards[i].type == XIANG_RI_KUI) {
			cards[i].cost = 50;
		}
		else if (cards[i].type == YING_TAO_ZHA_DAN) {
			cards[i].cost = 150;
		}
		else if (cards[i].type == JIANG_GUO_QIANG) {
			cards[i].cost = 50;
		}
		else if (cards[i].type == TU_DOU_LEI) {
			cards[i].cost = 25;
		}
	}
	curPlant = 0; // 初始化选中植物状态
	sunshine = 9999; // 初始化阳光值
	// 初始化阳光球数组
	memset(balls, 0, sizeof(balls));
	for (int i = 0; i < 29; i++) {
		sprintf_s(name, sizeof(name), "res/sunshine/%d.png", i + 1);
		loadimage(imgSunshineBall + i, name);
	}
	// 随机种子
	srand(time(NULL));
	// 创建游戏图形窗口
	initgraph(WIN_WIDTH, WIN_HEIGHT, 1);
	// 设置字体
	LOGFONT font;
	getfont(&font);
	font.lfHeight = 30;
	font.lfWeight = 15;
	strcpy(font.lfFaceName, "Segoe UI Black");
	font.lfQuality = ANTIALIASED_QUALITY; // 抗锯齿质量
	settextstyle(&font);
	setbkmode(TRANSPARENT);
	setcolor(BLACK);
	// 初始化僵尸行走的帧图片数组
	memset(zombies, 0, sizeof(zombies));
	for (int i = 0; i < 22; i++) {
		sprintf_s(name, sizeof(name), "res/zm/%d.png", i + 1);
		loadimage(&imgZombie[i], name);
	}
	// 初始化豌豆子弹的帧图片数组
	loadimage(&imgBulletNormal, "res/bullets/bullet_normal.png");
	memset(bullets, 0, sizeof(bullets));
	// 初始化豌豆子弹的破裂帧图片数组
	loadimage(&imgBulletBlast[3], "res/bullets/bullet_blast.png");
	for (int i = 0; i < 3; i++) {
		double k = (i + 1) * 0.2;
		loadimage(&imgBulletBlast[i], "res/bullets/bullet_blast.png", imgBulletBlast[3].getwidth() * k, imgBulletBlast[3].getheight() * k, 1);
	}
	// 初始化僵尸吃植物的帧图片数组
	for (int i = 0; i < 21; i++) {
		sprintf_s(name, sizeof(name), "res/zm_eat/%d.png", i + 1);
		loadimage(&imgZombieEating[i], name);
	}
	// 初始化僵尸死亡的帧图片数组
	for (int i = 0; i < 11; i++) {
		sprintf_s(name, sizeof(name), "res/zm_dead/%d.png", i + 1);
		loadimage(&imgZombieDead[i], name);
	}
	// 初始化植物因灰烬植物死亡的帧图片数组
	for (int i = 0; i < 21; i++) {
		sprintf_s(name, sizeof(name), "res/zm_dead2/%d.png", i + 1);
		loadimage(&imgZombieBoomDead[i], name);
	}
	// 初始化开场僵尸的帧图片数组
	for (int i = 0; i < 11; i++) {
		sprintf_s(name, sizeof(name), "res/zm_stand/%d.png", i + 1);
		loadimage(&imgZombieStand[i], name);
	}
	// 初始化铲子图片
	sprintf_s(name, sizeof(name), "res/Shovel/shovel.png");
	loadimage(&imgShovel, name);
	sprintf_s(name, sizeof(name), "res/Shovel/shovelSlot.png");
	loadimage(&imgShovelSlot, name);
	// 初始化小车图片
	sprintf_s(name, sizeof(name), "res/car.png");
	loadimage(&imgCar, name);
	// 小车状态设置
	for (int i = 0; i < 5; i++) {
		cars[i].exist = 1;
		cars[i].trigger = 0;
		cars[i].x = 70;
		cars[i].y = (i + 1) * 102 + 15;
		cars[i].speed = 0;
	}
	// 初始化坚果墙的其他两种形态的帧图片数组
	for (int i = 0; i < 11; i++) {
		sprintf_s(name, sizeof(name), "res/plants/3/half/%d.png", i + 1);
		loadimage(&imgWallnutHalf[i], name);
	}
	for (int i = 0; i < 15; i++) {
		sprintf_s(name, sizeof(name), "res/plants/3/quarter/%d.png", i + 1);
		loadimage(&imgWallnutQuarter[i], name);
	}
	// 初始化土豆雷的未准备图片、触发后的效果图片
	sprintf_s(name, sizeof(name), "res/plants/4/unready.png");
	loadimage(&imgUnready, name);
	sprintf_s(name, sizeof(name), "res/plants/4/xipu.png");
	loadimage(&imgXipu, name);
	sprintf_s(name, sizeof(name), "res/plants/4/xipu_text.png");
	loadimage(&imgXipuText, name);
}

void drawCards() {
	for (int i = 0; i < ZHI_WU_COUNT; i++) {
		int x = 268 - 112 + i * 65;
		int y = 6;
		// 未进入冷却时为彩色，反之为黑白色
		if (cards[i].able) {
			putimage(x, y, &imgCards[i]);
		}
		else putimage(x, y, &imgCardsBlack[i]);
	}
}

void drawPlant() {
	// 实现植物的逐帧摇摆，创造动感
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type > 0) {
				int PlantType = map[i][j].type - 1;
				int index = map[i][j].frameIndex;
				if (index == 0) {
					index++;
				}
				if (map[i][j].type == YING_TAO_ZHA_DAN + 1 && map[i][j].frameIndex >= 8) {
					putimagePNG(map[i][j].x - 70, map[i][j].y + 15 - 70, imgPlant[PlantType][index]);
				}
				else if (map[i][j].half) {
					putimagePNG(map[i][j].x, map[i][j].y + 15, &imgWallnutHalf[index]);
				}
				else if (map[i][j].quarter) {
					putimagePNG(map[i][j].x, map[i][j].y + 15, &imgWallnutQuarter[index]);
				}
				else if (map[i][j].type == TU_DOU_LEI + 1 && map[i][j].ready == 0) {
					putimagePNG(map[i][j].x, map[i][j].y + 15, &imgUnready);
				}
				else if (map[i][j].trigger) {
					putimagePNG(map[i][j].x, map[i][j].y + 15, &imgXipu);
					putimagePNG(map[i][j].x, map[i][j].y + 15, &imgXipuText);
				}
				else {
					putimagePNG(map[i][j].x, map[i][j].y + 15, imgPlant[PlantType][index]);
				}
			}
			if (map[i][j].type == YING_TAO_ZHA_DAN + 1 && map[i][j].frameIndex >= 19) {
				map[i][j].type = 0;
			}
		}
	}
	//渲染拖动过程的植物
	if (curPlant) {
		bool changeFlag = 1;
		for (int i = 0; i < 5; i++) {
			for (int j = 0; j < 9; j++) {
				if (unmap[i][j].type > 0 && map[i][j].type == 0) {
					unmap[i][j].x = 259 - 112 + j * 81;
					unmap[i][j].y = 77 + 15 + i * 102;
					IMAGE* unimg = imgPlant[unmap[i][j].type - 1][0];
					putimagePNG(unmap[i][j].x, unmap[i][j].y, unimg);
					if (unmap[i][j].x == checkUnmap.x && unmap[i][j].y == checkUnmap.y) {
						changeFlag = 0;
					}
					checkUnmap = unmap[i][j];
				}
			}
		}
		// 防频闪
		if (changeFlag) {
			memset(unmap, 0, sizeof(unmap));
		}
		IMAGE* img = imgPlant[curPlant - 1][1];
		putimagePNG(curX - img->getwidth() / 2, curY - img->getheight() / 2, img);
	}
	// 避免上一个未种植的植物影响
	else {
		memset(unmap, 0, sizeof(unmap));
	}
}

void drawSunshine() {
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	// 渲染产生或制造的阳光球
	for (int i = 0; i < ballMax; i++) {
		if (balls[i].used) {
			IMAGE* img = &imgSunshineBall[balls[i].frameIndex];
			putimagePNG(balls[i].currentPresentation.x, balls[i].currentPresentation.y, img);
		}
	}
	// 渲染收集的阳光球
	for (int i = 0; i < ballMax; i++) {
		if (balls[i].status == SUNSHINE_COLLECT) {
			IMAGE* img = &imgSunshineBall[balls[i].frameIndex];
			putimagePNG(balls[i].currentPresentation.x, balls[i].currentPresentation.y, img);
		}
	}
	//显示阳光值
	char scoreText[5];
	// 设置阳光值最大值
	if (sunshine >= 9999) {
		sunshine = 9999;
	}
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
				mciSendString("play res/zombeat.wav", 0, 0, 0);
			}
			else {
				img = imgZombie;
			}
			img += zombies[i].frameIndex;
			// 用于“玩玩小游戏：隐形食脑者” 待开发——
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

void drawShovel() {
	if (curShovel) {
		putimagePNG(curX - 5, curY - imgShovel.getheight() + 5, &imgShovel);
	}
	else {
		putimagePNG(685, 5, &imgShovel);
	}
}

void drawCar() {
	for (int i = 0; i < 5; i++) {
		if (cars[i].exist) {
			putimagePNG(cars[i].x, cars[i].y, &imgCar);
		}
	}
}

void updateWindow() {
	BeginBatchDraw();
	//渲染游戏背景、植物卡槽和铲子槽
	putimage(-112, 15, &imgBackground);
	putimagePNG(180 - 112, 0, &imgBar5);
	//putimagePNG(680, -5, &imgShovelSlot);
	drawCards();
	drawPlant();
	drawZombie();
	drawBullet();
	//drawShovel();
	//drawCar();
	drawSunshine();
	EndBatchDraw();
}

void updatePlant() {
	static int count = 0;
	if (++count < 3) {
		return;
	}
	count = 0;
	//判断阳光数是否足以购买植物以及卡牌是否冷却完毕
	for (int i = 0; i < ZHI_WU_COUNT; i++) {
		if (cards[i].CD == 0) {
			cards[i].able = 1;
			if (sunshine >= cards[i].cost) {
				cards[i].able = 1;
			}
			else {
				cards[i].able = 0;
			}
		}
		else {
			cards[i].able = 0;
			cards[i].CD--;
		}
	}
	//更新摇摆帧
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type > 0) {
				if (map[i][j].half) {
					map[i][j].frameIndex = (map[i][j].frameIndex + 1) % 11;
				}
				else if (map[i][j].quarter) {
					map[i][j].frameIndex = (map[i][j].frameIndex + 1) % 15;
				}
				else if (map[i][j].type == TU_DOU_LEI + 1 && map[i][j].ready == 0) {
					map[i][j].readyTime++;
					if (map[i][j].readyTime > 100) {
						map[i][j].ready = 1;
					}
				}
				else {
					map[i][j].frameIndex++;
					int PlantType = map[i][j].type - 1;
					int index = map[i][j].frameIndex;
					if (imgPlant[PlantType][index] == NULL) {
						map[i][j].frameIndex = 0;
					}
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
		// 防越界
		if (i >= ballMax) {
			return;
		}
		balls[i].used = 1;
		balls[i].frameIndex = 0;
		balls[i].timer = 0;
		balls[i].status = SUNSHINE_DOWN;
		balls[i].t = 0;
		balls[i].p1 = vector2(260 - 112 + rand() % (900 - 260), 60);
		balls[i].p4 = vector2(balls[i].p1.x, 200 + (rand() % 4) * 90);
		double off = 2.0;
		double distance = balls[i].p4.y - balls[i].p1.y;
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
						// 防越界
						if (k >= ballMax) {
							return;
						}
					}
					// 利用贝塞尔曲线勾勒阳光球制造出的路径
					balls[k].used = 1;
					balls[k].p1 = vector2(map[i][j].x, map[i][j].y);
					int w = (30 + rand() % 15) * (rand() % 2 ? 1 : -1);
					balls[k].p4 = vector2(map[i][j].x + w,
						map[i][j].y + imgPlant[XIANG_RI_KUI][0]->getwidth() - imgSunshineBall[0].getwidth());
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
			// 自然产生的阳光球下落
			if (balls[i].status == SUNSHINE_DOWN) {
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->currentPresentation = sun->p1 + sun->t * (sun->p4 - sun->p1);
				if (sun->t >= 1) {
					sun->status = SUNSHINE_GROUND;
					sun->timer = 0;
				}
			}
			// 阳光球停止一段时间后如未及时点击则消失
			else if (balls[i].status == SUNSHINE_GROUND) {
				balls[i].timer++;
				if (balls[i].timer > 200) {
					balls[i].timer = 0;
					balls[i].used = 0;
				}
			}
			// 利用贝塞尔曲线勾勒阳光球制造出的路径
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
		// 收集阳光
		else if (balls[i].status == SUNSHINE_COLLECT) {
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
	// 特判停止出现僵尸
	if (zombieCount >= ZOMBIE_MAX) {
		return;
	}
	// 僵尸来临音效
	static bool zombiesAreComing = 0;
	if (zombieCount == 1 && zombiesAreComing == 0) {
		zombiesAreComing = 1;
		mciSendString("play res/zombiesAreComing.wav", 0, 0, 0);
	}
	// 控制僵尸出现频率
	static int zombieFrequence = 500;
	static int count = 0;
	count++;
	if (count > zombieFrequence) {
		count = 0;
		zombieFrequence = 300 + rand() % 200;
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
	// 僵尸间歇性呻吟音效
	static int count2 = 0;
	if (zombiesAreComing && zombieCount > killCount) {
		count2++;
	}
	if (count2 > 800) {
		count2 = 0;
		int num = rand() % 6;
		switch (num)
		{
		case 0:
			mciSendString("play res/groan1.mp3", 0, 0, 0);
		case 1:
			mciSendString("play res/groan2.mp3", 0, 0, 0);
		case 2:
			mciSendString("play res/groan3.mp3", 0, 0, 0);
		case 3:
			mciSendString("play res/groan4.mp3", 0, 0, 0);
		case 4:
			mciSendString("play res/groan5.mp3", 0, 0, 0);
		case 5:
			mciSendString("play res/groan6.mp3", 0, 0, 0);
		}
	}
}

void updateZombie() {
	int zombieMax = sizeof(zombies) / sizeof(zombies[0]);
	// 更新僵尸位置
	static int count1 = 0;
	count1++;
	if (count1 > 2) {
		count1 = 0;
		for (int i = 0; i < zombieMax; i++) {
			if (zombies[i].used && zombies[i].dead == 0 && zombies[i].boomDead == 0) {
				zombies[i].x -= zombies[i].speed;
				if (zombies[i].x < 60) {
					if (zombies[i].x < 44 && cars[zombies[i].row].exist == 0) {
						// 判定失败
						gameStatus = FAIL;
					}
					else {
						cars[zombies[i].row].trigger = 1;
					}
				}
			}
		}
	}
	// 更新僵尸状态（行走、啃食、死亡及死亡后倒下/化灰）
	static int count2 = 0;
	count2++;
	if (count2 > 2) {
		count2 = 0;
		for (int i = 0; i < zombieMax; i++) {
			if (zombies[i].used) {
				if (zombies[i].dead) {
					zombies[i].frameIndex++;
					// 如果僵尸死亡就增加死亡的帧数，并在加完之后删除僵尸
					if (zombies[i].frameIndex >= 11) {
						zombies[i].used = 0;
						killCount++;
					}
				}
				else if (zombies[i].boomDead) {
					zombies[i].frameIndex++;
					if (zombies[i].frameIndex >= 20) {
						zombies[i].used = 0;
						killCount++;
					}
				}
				else if (zombies[i].eating) {
					zombies[i].frameIndex = (zombies[i].frameIndex + 1) % 21;
				}
				else {
					zombies[i].frameIndex = zombies[i].frameIndex % 21 + 1;
				}
			}
		}
	}
}

void shoot() {
	static int count = 0;
	if (++count < 3) {
		return;
	}
	count = 0;
	int lines[5] = { 0 };
	int zombieMax = sizeof(zombies) / sizeof(zombies[0]);
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
	int dangerX = WIN_WIDTH - 50;
	// 判断僵尸所在的行
	for (int i = 0; i < zombieMax; i++) {
		if (zombies[i].used && zombies[i].x < dangerX) {
			lines[zombies[i].row] = 1;
		}
	}
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			// 遍历每行每列判断植物是否为豌豆射手
			if (map[i][j].type == WAN_DOU + 1 && lines[i]) {
				for (int k = 0; k < zombieMax; k++) {
					// 遍历每个僵尸是否进入射程
					if (zombies[k].row == i && map[i][j].x < zombies[k].x) {
						// 控制发射频率
						map[i][j].shootTime++;
						if (map[i][j].shootTime > 20) {
							map[i][j].shootTime = 0;
							int k = 0;
							// 遍历每个豌豆子弹对其进行状态更新
							for (k = 0; k < bulletMax && bullets[k].used; k++);
							if (k < bulletMax) {
								bullets[k].used = 1;
								bullets[k].row = i;
								bullets[k].speed = 24;
								bullets[k].blast = 0;
								bullets[k].frameIndex = 0;
								int plantX = 256 - 112 + j * 81;
								int plantY = 77 + i * 102 + 27;
								bullets[k].x = plantX + imgPlant[map[i][j].type - 1][0]->getwidth() - 10;
								bullets[k].y = plantY;
							}
						}
						// 优化、降低时间复杂度
						break;
					}
				}
			}
		}
	}
}

void updateBullet() {
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
	static int count = 0;
	if (++count < 2) {
		return;
	}
	count = 0;
	for (int i = 0; i < bulletMax; i++) {
		if (bullets[i].used) {
			// 更新子弹位置
			bullets[i].x += bullets[i].speed;
			// 子弹超出屏幕后回收子弹
			if (bullets[i].x > WIN_WIDTH) {
				bullets[i].used = 0;
			}
			// 子弹破裂后回收子弹
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
		if (bullets[i].used == 0 || bullets[i].blast) {
			continue;
		}
		for (int j = 0; j < zombieMax; j++) {
			if (zombies[j].used == 0) {
				continue;
			}
			int x1 = zombies[j].x + 80;
			int x2 = zombies[j].x + 110;
			int x = bullets[i].x;
			if (bullets[i].row == zombies[j].row && x > x1 && x < x2 && zombies[j].dead == 0) {
				mciSendString("play res/attrackZomb.mp3", 0, 0, 0);
				zombies[j].blood -= 10;
				bullets[i].blast = 1;
				bullets[i].speed = 0;
				// 僵尸死亡判断（一般死亡）
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
			int plantX = 256 - 112 + j * 81;
			int x1 = plantX + 10;
			int x2 = plantX + 60;
			int x3 = zombies[i].x + 80;
			if (x3 > x1 && x3 < x2) {
				// 修复多个僵尸同时将植物啃食完毕后僵尸仍在啃食的bug
				if (map[row][j].type == 0) {
					if (zombies[i].eating) {
						zombies[i].eating = 0;
						zombies[i].speed = (100 + rand() % 50) / 100;
					}
					continue;
				}
				zombies[i].eating = 1;
				zombies[i].speed = 0;
				for (int k = 0; k < zombieMax; k++) {
					// 植物被啃食时进行死亡倒计时，灰烬植物默认无敌
					if (zombies[k].eating && map[row][j].type != YING_TAO_ZHA_DAN + 1) {
						map[row][j].deadTime++;
					}
					if (map[row][j].deadTime > 150) {
						// 坚果墙特殊处理，死亡较慢
						if (map[row][j].type == JIANG_GUO_QIANG + 1) {
							if (map[row][j].deadTime > 600 && map[row][j].deadTime <= 1200) {
								map[row][j].half = 1;
							}
							else if (map[row][j].deadTime > 1200 && map[row][j].deadTime <= 1500) {
								map[row][j].half = 0;
								map[row][j].quarter = 1;
							}
							else if (map[row][j].deadTime > 1500) {
								map[row][j].quarter = 0;
								map[row][j].deadTime = 0;
								map[row][j].type = 0;
								zombies[i].eating = 0;
								zombies[i].speed = (100 + rand() % 50) / 100;
							}
						}
						else {
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
}

void car2Zombie() {
	int zombieMax = sizeof(zombies) / sizeof(zombies[0]);
	for (int i = 0; i < 5; i++) {
		if (cars[i].trigger) {
			mciSendString("play res/carQiDong.mp3", 0, 0, 0);
			int x = cars[i].x + 60;
			for (int j = 0; j < zombieMax; j++) {
				int x1 = zombies[j].x + 80;
				int x2 = zombies[j].x + 100;
				if (zombies[j].row == i && x >= x1 && x <= x2) {
					zombies[j].dead = 1;
					zombies[j].speed = 0;
				}
			}
		}
	}
}

void potato_mine2Zombie() {
	int zombieMax = sizeof(zombies) / sizeof(zombies[0]);
	for (int i = 0; i < zombieMax; i++) {
		int row = zombies[i].row;
		for (int j = 0; j < 9; j++) {
			if (map[row][j].type == TU_DOU_LEI + 1 && map[row][j].ready) {
				int x = map[row][j].x + 75;
				int x1 = zombies[i].x + 80;
				int x2 = zombies[i].x + 100;
				if (x > x1 && x < x2) {
					map[row][j].trigger = 1;
				}
			}
		}
	}
}

void collisionCheck() {
	bullet2Zombie();
	car2Zombie();
	potato_mine2Zombie();
	zombie2Plant();
}

void updateNearbyZombies(int type, int boomX, int boomY) {
	int zombieMax = sizeof(zombies) / sizeof(zombies[0]);
	for (int i = 0; i < zombieMax; i++) {
		if (zombies[i].used && type == YING_TAO_ZHA_DAN + 1) {
			if (abs(zombies[i].x - boomX) <= 110 && abs(zombies[i].y - boomY) <= 110) {
				zombies[i].boomDead = 1;
			}
		}
		else if (zombies[i].used && type == TU_DOU_LEI + 1) {
			if (abs(zombies[i].x - boomX) <= 50 && abs(zombies[i].y - boomY) <= 100) {
				zombies[i].boomDead = 1;
			}
		}
	}
}

void boom() {
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type == YING_TAO_ZHA_DAN + 1 && map[i][j].frameIndex == 8) {
				mciSendString("play res/cherrybomb.mp3", 0, 0, 0);
				updateNearbyZombies(map[i][j].type, map[i][j].x - 60, map[i][j].y + 102);
			}
			if (map[i][j].trigger) {
				updateNearbyZombies(map[i][j].type, map[i][j].x, map[i][j].y + 102);
				mciSendString("play res/potato_mine.mp3", 0, 0, 0);
				map[i][j].type = 0;
				map[i][j].trigger = 0;
				map[i][j].ready = 0;
			}
		}
	}
}

void updateCar() {
	for (int i = 0; i < 5; i++) {
		if (cars[i].exist && cars[i].trigger) {
			cars[i].speed = 10;
			cars[i].x += cars[i].speed;
			if (cars[i].x > WIN_WIDTH) {
				cars[i].exist = 0;
				cars[i].trigger = 0;
			}
		}
	}
}

void judgeWin() {
	if (killCount == ZOMBIE_MAX) {
		int j = 0;
		//确保小车全部跑完，xixi~~~
		for (j = 0; j < 5; j++) {
			if (cars[j].trigger) {
				break;
			}
		}
		if (j == 5) {
			gameStatus = WIN;
		}
	}
}

void updateGame() {
	updatePlant();
	createSunshine();
	updateSunshine();
	createZombie();
	updateZombie();
	shoot();
	updateBullet();
	boom();
	updateCar();
	collisionCheck();
	judgeWin();
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
				double distance = dis(balls[i].p1 - balls[i].p4);
				double off = 8.0;
				balls[i].speed = 3.0 / (distance / off);
				break;
			}
		}
	}
}

void userClick() {
	ExMessage msg;
	if (peekmessage(&msg)) {
		// 鼠标左键点击设定（未选取）
		if (msg.message == WM_LBUTTONDOWN && firstDown == 0) {
			if (msg.x > 268 - 112 && msg.x < 268 - 112 + 65 * ZHI_WU_COUNT && msg.y < 96) { // 点击植物卡牌
				int index = (msg.x - 268 + 112) / 65;
				if (cards[index].able) {
					mciSendString("play res/seedlift.mp3", 0, 0, 0);
					curPlant = index + 1;
					firstDown = 1;
					curX = msg.x;
					curY = msg.y;
				}
			}
			else if (msg.x > 680 && msg.x < 680 + 90 && msg.y < 95) { // 点击铲子
				mciSendString("play res/shovel.mp3", 0, 0, 0);
				curShovel = 1;
				firstDown = 1;
				curX = msg.x;
				curY = msg.y;
			}
			else collectSunshine(&msg); // 收集阳光
		}
		// 鼠标右键点击设定
		else if (msg.message == WM_RBUTTONDOWN && firstDown == 1) { // 取消当前选取
			firstDown = 0;
			curPlant = 0;
			curShovel = 0;
		}
		// 鼠标移动追踪
		else if (msg.message == WM_MOUSEMOVE && (curPlant || curShovel)) {
			curX = msg.x;
			curY = msg.y;
			if (msg.x > 256 - 112 && msg.x < 985 - 112 && msg.y > 77 + 15 && msg.y < 591 + 15) {
				int row = (msg.y - 77) / 102;
				int col = (msg.x - 256 + 112) / 81;
				unmap[row][col].type = curPlant;
			}
		}
		// 鼠标左键点击设定（已选取）
		else if (msg.message == WM_LBUTTONDOWN && firstDown == 1) {
			if (msg.x > 256 - 112 && msg.x < 985 - 112 && msg.y > 77 + 15 && msg.y < 591 + 15) {
				int row = (msg.y - 77) / 102;
				int col = (msg.x - 256 + 112) / 81;
				// 种下植物
				if (map[row][col].type == 0 && curPlant) {
					mciSendString("play res/grow.mp3", 0, 0, 0);
					map[row][col].x = 256 - 112 + col * 81;
					map[row][col].y = 77 + row * 102;
					map[row][col].type = curPlant;
					map[row][col].frameIndex = 0;
					map[row][col].shootTime = 0;
					// 根据所种植的植物更新特殊状态和CD
					if (map[row][col].type == YING_TAO_ZHA_DAN + 1) {
						cards[YING_TAO_ZHA_DAN].CD = 500;
					}
					else if (map[row][col].type == WAN_DOU + 1) {
						cards[WAN_DOU].CD = 200;
					}
					else if (map[row][col].type == XIANG_RI_KUI + 1) {
						cards[XIANG_RI_KUI].CD = 200;
					}
					else if (map[row][col].type == JIANG_GUO_QIANG + 1) {
						cards[JIANG_GUO_QIANG].CD = 500;
					}
					else if (map[row][col].type == TU_DOU_LEI + 1) {
						cards[TU_DOU_LEI].CD = 500;
					}
					sunshine -= cards[curPlant - 1].cost;
				}
				// 铲掉植物
				else if (map[row][col].type && curShovel) {
					map[row][col].type = 0;
					curShovel = 0;
				}
				else if (curShovel) {
					curShovel = 0;
				}
				// 恢复
				firstDown = 0;
				curPlant = 0;
			}
		}
	}
}

void startUI() {
	IMAGE imgBackground, imgQuit[2], imgHelp[3], imgMenu[9];
	char name[64];
	bool flag[7] = { 0 }, check = 0, check2 = 0;
	//加载菜单界面各元素图片
	loadimage(&imgBackground, "res/menu.png");
	for (int i = 1; i <= 8; i++) {
		sprintf_s(name, sizeof(name), "res/menu%d.png", i);
		loadimage(imgMenu + i, name);
	}
	sprintf_s(name, sizeof(name), "res/quit1.png");
	loadimage(imgQuit, name);
	sprintf_s(name, sizeof(name), "res/quit2.png");
	loadimage(imgQuit + 1, name);
	sprintf_s(name, sizeof(name), "res/help1.png");
	loadimage(imgHelp, name);
	sprintf_s(name, sizeof(name), "res/help2.png");
	loadimage(imgHelp + 1, name);
	sprintf_s(name, sizeof(name), "res/helpContent.png");
	loadimage(imgHelp + 2, name);
	while (1) {
		mciSendString("play res/bg.wav", 0, 0, 0);
		BeginBatchDraw();
		putimage(0, 0, &imgBackground);
		// 单击高亮
		for (int i = 1; i <= 4; i++) {
			putimagePNG(474, -30 + i * 90, flag[i] ? imgMenu + 2 * i : imgMenu + 2 * i - 1);
		}
		putimagePNG(810, 510, flag[0] ? imgQuit + 1 : imgQuit);
		putimagePNG(730, 520, flag[5] ? imgHelp + 1 : imgHelp);
		// 帮助页面查看
		if (flag[6]) {
			putimage(0, 0, imgHelp + 2);
		}
		ExMessage msg;
		// 消息处理
		if (peekmessage(&msg)) {
			if (flag[6]) {
				if (msg.message == WM_LBUTTONDOWN && msg.x > 367 && msg.x < 367 + 175 && msg.y > 523 && msg.y < 523 + 90) {
					check2 = 1;
				}
				else if (msg.message == WM_LBUTTONUP && msg.x > 367 && msg.x < 367 + 175 && msg.y > 523 && msg.y < 523 + 90 && check2) {
					mciSendString("play res/buttonclick.mp3", 0, 0, 0);
					flag[6] = 0;
				}
			}
			else if (msg.message == WM_LBUTTONDOWN) {
				if (msg.x > 810 && msg.x < 810 + 47 && msg.y > 510 && msg.y < 510 + 27) {
					flag[0] = 1;
					check = 1;
				}
				if (msg.x > 730 && msg.x < 730 + 47 && msg.y > 520 && msg.y < 520 + 27) {
					flag[5] = 1;
					check = 1;
				}
				for (int i = 1; i <= 4; i++) {
					if (msg.x > 474 && msg.x < 474 + 300 && msg.y > -30 + 90 * i && msg.y < -30 + 90 * (i + 1)) {
						flag[i] = 1;
						check = 1;
					}
				}
			}
			else if (msg.message == WM_LBUTTONUP && check) {
				// 查看帮助
				if (msg.x > 730 && msg.x < 730 + 47 && msg.y > 520 && msg.y < 520 + 27) {
					mciSendString("play res/buttonclick.mp3", 0, 0, 0);
					flag[6] = 1;
					flag[5] = 0;
				}
				else {
					flag[5] = 0;
				}
				// 退出
				if (msg.x > 810 && msg.x < 810 + 47 && msg.y > 510 && msg.y < 510 + 27) {
					mciSendString("play res/buttonclick.mp3", 0, 0, 0);
					exit(0);
				}
				else {
					flag[0] = 0;
				}
				// 进入游戏
				for (int i = 1; i <= 4; i++) {
					if (msg.x > 474 && msg.x < 474 + 300 && msg.y > -30 + 90 * i && msg.y < -30 + 90 * (i + 1)) {
						mciSendString("play res/buttonclick.mp3", 0, 0, 0);
						mciSendString("stop res/bg.wav", 0, 0, 0);
						mciSendString("close res/bg.wav", 0, 0, 0);
						return;
					}
					else {
						flag[i] = 0;
					}
				}
			}
		}
		EndBatchDraw();
	}
}

void viewScene() {
	mciSendString("play res/menumusic.wav", 0, 0, 0);
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
		if (count >= 10) {
			count = 0;
		}
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
				if (count >= 10) {
					count = 0;
				}
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
	mciSendString("stop res/menumusic.wav", 0, 0, 0);
	mciSendString("close res/menumusic.wav", 0, 0, 0);
}

bool checkOver() {
	bool ret = false;
	// 顺利通关~~~
	if (gameStatus == WIN) {
		Sleep(2000);
		loadimage(0, "res/win2.png");
		mciSendString("play res/gamewin.wav", 0, 0, 0);
		ret = true;
	}
	// 遗憾退败~~~
	else if (gameStatus == FAIL) {
		Sleep(2000);
		loadimage(0, "res/fail2.png");
		mciSendString("play res/losemusic.mp3", 0, 0, 0);
		ret = true;
	}
	return ret;
}

int main() {
	gameInit();
	startUI();
	viewScene();
	barDown();
	int timer = 0;
	while (1) {
		userClick();
		mciSendString("play res/zombrun.wav", 0, 0, 0);
		//延迟输出，减缓更新速度
		timer += getDelay();
		if (timer > 30) {
			timer = 0;
			updateWindow();
			updateGame();
			if (checkOver()) {
				mciSendString("stop res/zombrun.wav", 0, 0, 0);
				mciSendString("close res/zombrun.wav", 0, 0, 0);
				break;
			}
		}
	}
	Sleep(5000);
	return 0;
}