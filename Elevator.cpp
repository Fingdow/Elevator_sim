#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <ctime>
#include <windows.h>
#include <conio.h>

using namespace std;

const int floor_max = 20;

class Person {
public:
	Person(int destination, int tolerance) :dest(destination), tol(tolerance) {}
	int dest, tol;
};

class Elevator {
public:
	Elevator(int total_floor = 5, int height = 4);
	~Elevator();
	bool add_person(int at, int dest, int tolerance = -1);
	void rand_wait_person(int num);
	void show();
	void move();
	void auto_run();
	void test() {
		cout << "status = " << status << endl;
		cout << "pos = " << pos << endl;
		cout << "at_floor = " << at_floor(pos) << endl;

		cout << "up_button:   ";
		for (int i = 1; i <= total_floor; i++)
			cout << up_button[i] << " ";
		cout << endl;

		cout << "down_button: ";
		for (int i = 1; i <= total_floor; i++)
			cout << down_button[i] << " ";
		cout << endl;

		cout << "in_button:   ";
		for (int i = 1; i <= total_floor; i++)
			cout << in_button[i] << " ";
		cout << endl;
	}
	bool show_with_tolerance; //是否显示每个人的等待忍受时间

private:
	vector<Person> passenger, *wait_line;

	bool* open, * in_button, * up_button, * down_button; //标记各楼层的按钮和门的状态

	const char* block0 = "  ";
	const char* block1 = "█";
	const char* block2 = "▓";

	int timescale;			//auto_run()的刷新周期(ms)
	int total_floor;		//电梯总层数
	int height;				//每一楼层的像素高度(包括楼层隔板)
	int pos;				//电梯所在的行数
	int line_max;			//打印的最后一行的序号(从0计数)
	int load_max;			//电梯最大载客数
	int wait_line_max;		//显示的最大等待队伍长度
	int default_tolerance;	//基础等待忍受时间

	enum elev_status { up = -1, no = 0, down = 1 } status;
	string num_icon = "  ①②③④⑤⑥⑦⑧⑨⑩⑾⑿⒀⒁⒂⒃⒄⒅⒆⒇";

	void del(vector<Person>& vec, int pos);
	void print_deq(const vector<Person>& deq, int show_length);
	void print_tolerance(const vector<Person>& deq);

	inline int at_floor(int pos);
	bool nothing_to_do();
	bool stop_sign(int floor);
	void update_status();
	void update_tolerance();
	bool check_in(int floor);
	bool check_out(int floor);
	bool movein(int floor);
	void moveout(int num);

	void repeat_print(const char* str, int times, bool endline = false) {
		while (times--) printf("%s", str);
		if (endline) printf("\n");
	}

};

Elevator::Elevator(int _total_floor, int _height) {
	//常量初始化
	timescale = 200; //ms
	total_floor = max(2, min(floor_max, _total_floor));
	height = max(2, min(10, _height));
	pos = total_floor * height;
	line_max = pos + 1;
	load_max = 9;
	wait_line_max = 9;
	default_tolerance = pos * 2 + 10;
	status = no;
	show_with_tolerance = true;
	//内存空间申请和初始化
	int n = total_floor + 1;
	wait_line = new vector<Person>[n];
	open = new bool[n];
	in_button = new bool[n];
	up_button = new bool[n];
	down_button = new bool[n];
	for (int i = 0; i <= n; ++i) open[i] = in_button[i] = up_button[i] = down_button[i] = false;
}

Elevator::~Elevator() {
	delete[] open;
	delete[] in_button;
	delete[] up_button;
	delete[] down_button;
	delete[] wait_line;
}

//给定下标删除向量中的元素
void Elevator::del(vector<Person>& vec, int pos) {
	vector<Person>::iterator it = vec.begin() + pos;
	vec.erase(it);
}

//打印向量
void Elevator::print_deq(const vector<Person>& deq, int show_length) {
	int len = deq.size();
	for (int i = 0; i < show_length; ++i) {
		if (i >= len) printf("  ");
		else cout << num_icon.substr(deq[i].dest * 2, 2);
	}
}

//打印剩余忍受时间
void Elevator::print_tolerance(const vector<Person>& deq) {
	int len = deq.size();
	for (int i = 0; i < wait_line_max; ++i) {
		if (i >= len) printf("  ");
		else printf("%2d", deq[i].tol);
	}

}

//计算当前所在楼层
inline int Elevator::at_floor(int pos) {
	return (line_max - pos - 1) / height + 1;
}

//检测是否存在请求信号
bool Elevator::nothing_to_do() {
	for (int i = 1; i <= total_floor; ++i)
		if (in_button[i] || up_button[i] || down_button[i] || open[i] == true) return false;
	return true;
}

//检测停留信号
bool Elevator::stop_sign(int floor) {
	if (in_button[floor]) return true; //电梯内有人要去这层
	if (!up_button[floor] && !down_button[floor]) return false; //该楼无人
	if ((int)passenger.size() >= load_max) return false; //电梯满载
	else {
		if (status == up && up_button[floor]) return true; //顺路往上
		else if (status == down && down_button[floor]) return true; //顺路往下
		return false;
	}
}

//检测进电梯信号
bool Elevator::check_in(int floor) {
	if (int(passenger.size()) >= load_max) return false; //满载
	return (status == up && up_button[floor] || status == down && down_button[floor]);
}

//检测出电梯信号
bool Elevator::check_out(int floor) {
	return in_button[floor];
}

//让人进电梯并返回是否所有顺路的等待者都已进入
bool Elevator::movein(int floor) {
	vector<Person>& tmp = wait_line[floor];
	int len = tmp.size();
	int at = at_floor(pos);
	for (int i = 0; i < len;) {
		if (status == up && tmp[i].dest > at || status == down && tmp[i].dest < at) {
			if (passenger.size() == load_max) return false;
			passenger.push_back(tmp[i]);
			in_button[tmp[i].dest] = true;
			del(tmp, i);
			--len;
		}
		else ++i;
	}
	return true;
}

//让电梯内要去某层的人蒸发
void Elevator::moveout(int num) {
	for (vector<Person>::iterator it = passenger.begin(); it != passenger.end();)
		if ((*it).dest == num) it = passenger.erase(it);
		else ++it;
}

//运行规则写在这了
void Elevator::update_status() {
	int at = at_floor(pos);
	if (at == total_floor) status = down;
	else if (at == 1) {
		if (nothing_to_do()) status = no;
		else status = up;
	}
	else if (nothing_to_do()) status = down; //无事可做电梯会回到一楼	
	else {
		//电梯原运行方向优先
		if (status == up && up_button[at]) return;
		if (status == down && down_button[at]) return;

		//如果电梯内的请求已经处理完毕
		if (passenger.empty()) {
			if (up_button[at] || down_button[at]) {
				//电梯位于高层时优先响应更高楼层的下降请求
				if (at >= total_floor / 2 + 1 && down_button[at]) {
					for (int i = at + 1; at <= total_floor; ++at)
						if (down_button[i]) { status = up; return; }
					status = down;
				}
				//电梯位于低层时优先响应更低楼层的上升请求
				else if (at <= (total_floor + 1) / 2 && up_button[at]) {
					for (int i = at - 1; at >= 1; --at)
						if (up_button[i]) { status = down; return; }
					status = up;
				}
				//优先响应等待队伍第一个人的请求
				else status = wait_line[at][0].dest > at ? up : down;
			}
			return;
		}

		//如果之后的楼层都没有请求则改变运行方向
		while (1) {
			at += status == up ? 1 : -1;
			if (at > total_floor || at < 1) break;
			if (check_out(at) || up_button[at] || down_button[at]) return;
		}
		status = status == up ? down : up;
	}
}

//让忍耐不住的人消失
void Elevator::update_tolerance() {
	//(考虑到实际，被按的按钮不会取消，这意味着电梯可能被鸽)
	for (int i = 1; i <= total_floor; ++i) {
		vector<Person>& tmp = wait_line[i];
		for (int i = tmp.size() - 1; i >= 0; --i) {
			if (tmp[i].tol-- <= 0) {
				del(tmp, i);
			}
		}
	}
}

//让电梯进入下一个时间状态
void Elevator::move() {
	update_tolerance();
	if (pos % height == 0) { //如果当前到达某一楼层
		update_status();
		int at = at_floor(pos); //获取当前所在楼层
		//如果该楼电梯门是打开的
		if (open[at]) {
			if (check_out(at)) { //先出
				in_button[at] = false;
				moveout(at);
			}
			else if (check_in(at)) { //后进
				if (movein(at)) {
					//如果等待的人都进入电梯了相应的按钮就不亮了
					if (status == up) up_button[at] = false;
					else if (status == down) down_button[at] = false;
				}
			}
			else {
				open[at] = false; //关门
				update_status();
			}
		}
		//该楼电梯门是关闭的
		else {
			if (stop_sign(at)) { //检测到停留信号
				open[at] = true;
			}
			else pos += status; //离开
		}
	}
	else pos += status;
}

bool Elevator::add_person(int at, int dest, int tolerance) {
	if (at == dest) return false;
	if (tolerance <= 0) tolerance = default_tolerance;
	if (at > total_floor || at < 1 || dest > total_floor || dest < 1) return false;
	wait_line[at].push_back(Person(dest, tolerance));
	if (dest > at) up_button[at] = true;
	else down_button[at] = true;
	return true;
}

void Elevator::rand_wait_person(int num) {
	srand((unsigned int)time(0));
	while (num--) {
		int floor = rand() % total_floor + 1;
		int tol = default_tolerance + rand() % (default_tolerance / 2);
		while (1) if (add_person(floor, rand() % total_floor + 1, tol)) break;
	}
}

void Elevator::show() {
	system("cls");
	for (int i = 0; i <= line_max; ++i) {
		if (i == 0 || i == line_max) repeat_print(block1, 3 + load_max + wait_line_max, true);
		else {
			printf(block1); //左侧墙ok
			if (i == pos) repeat_print(block2, load_max); //电梯板
			else if (i + 1 == pos) print_deq(passenger, load_max); //乘客
			else repeat_print(block0, load_max); //空气
			//
			if (i % height == 0) repeat_print(block1, 2 + wait_line_max, true); //楼层隔板
			else {
				int at = at_floor(i);
				if (open[at]) printf(block0); else printf(block1); //电梯门ok
				if (i % height == height - 1) print_deq(wait_line[at], wait_line_max); //等待队伍ok
				else if (show_with_tolerance && i % height == height - 2) print_tolerance(wait_line[at]); //忍受时间ok
				else repeat_print(block0, wait_line_max); //空气
				printf(block1); printf("\n"); //右侧墙ok
			}
		}
	}
}

void Elevator::auto_run() {
	while (1) {
		if (rand() % (total_floor + 1) == 0) rand_wait_person(1);
		move();
		show();
		//test();
		Sleep(timescale);
	}
}

int main() {
	Elevator tmp(5, 4);
	tmp.rand_wait_person(10);
	tmp.auto_run();
}

