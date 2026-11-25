#pragma once
#include <glad/glad.h>
#include <stb_image.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
class Image
{
public:
	int width, height, nrChannels;
	unsigned char* data;
	Image(const char* path)
	{
		data = stbi_load(path, &width, &height, &nrChannels, 0);
	}
	void use2D(const char*mod)
	{
		if (data&&mod=="RGB")
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else if (data && mod == "RGBA")
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			std::cout <<data<< "Failed to load texture" << std::endl;
		}
	}
	~Image()
	{
		if(data) stbi_image_free(data);
	}

private:

};
