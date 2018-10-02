/*
 *  Created on: 2018/09/29
 *      Author: take
 */

#include "MinDxrCore.h"
#include <fstream>
#include <iostream>

int main(int argc, char **argv) {
	MinDxr::MinDxrCore core;
	core.Initialize();
	auto scene = std::make_shared<MinDxr::Scene>();
	scene->Init_Dummy();
	core.BuildScene(scene);
	core.ExecuteRaytracing();
	MinDxr::Image img(1280,720);
	core.GetResult(img);

	{
		std::ofstream out(L"result.bin", std::ios::binary);
		out.write(reinterpret_cast<char*>(img.GetData()), img.GetSize());
	}
	{
		std::ofstream out(L"result.ppm");
		out << "P3" << std::endl;
		out << 1280 << " " << 720 << std::endl;
		out << 255 << std::endl;
		auto pix = img.GetData();
		for (auto i = 0; i < 1280 * 720; i++) {
			auto p = pix[i];
			auto r = (p >> 0) & 0xff;
			auto g = (p >> 8) & 0xff;
			auto b = (p >> 16) & 0xff;
			out << r << " " << g << " " << b << std::endl;
		}
		out.flush();
	}

	return 0;
}



