//Author Jonathan Leung
#include "stdafx.h"
#include "FontHandler.h"

using namespace HARIKEN;
//Singleton unique pointer
std::unique_ptr<FontHandler> FontHandler::fontInstance = nullptr;
//Creates a FreeType font face
bool HARIKEN::FontHandler::createFont(const std::string & fontFileName_)
{

	if (Fonts[fontFileName_] != NULL)
		return true;

	FT_Face face;

	if (FT_New_Face(ft, fontFileName_.c_str(), 0, &face)) {

		std::cout << "!!ERROR!!" << std::endl << "FREETYPE: Failed to load font" << std::endl;

		return false;

	}

	else
		Fonts[fontFileName_] = face;

	return true;

}
//Generates and returns a character map needed by the text to be rendered
std::map<GLchar, Character>* HARIKEN::FontHandler::getCharacterMap(const std::string & fontName_, int fontSize_, const char* text_)
{
	//Set GPU pixel mode
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	//If the font for this text has not been created, return nullptr
	if (Fonts[fontName_] == nullptr)
		return nullptr;
	
	FT_Set_Pixel_Sizes(Fonts[fontName_], 0, fontSize_);
	//If an map exists for this particular font and size, then add onto that map, otherwise create a new map
	if (characterMap.find(fontName_ + std::to_string(fontSize_)) == characterMap.end())
		characterMap[fontName_ + std::to_string(fontSize_)] = new std::map<GLchar, Character>();

	std::map<GLchar, Character>* temp = characterMap[fontName_ + std::to_string(fontSize_)];

	const char *ch;
	//Cycle through the characters in the text to be rendered
	for (ch = text_; *ch; ch++)
	{
		//If texture for that character exists, move on to the next
		if (temp->find(*ch) != temp->end())
			continue;

		// Load character glyph 
		if (FT_Load_Char(Fonts[fontName_], *ch, FT_LOAD_RENDER))
		{
			std::cout << "!!ERROR!!" << std::endl << "Failed to load Glyph at " << ch << std::endl;
			continue;
		}
		// Generate texture
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			Fonts[fontName_]->glyph->bitmap.width,
			Fonts[fontName_]->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			Fonts[fontName_]->glyph->bitmap.buffer
		);
		// Set texture options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// Now store character for later use
		Character character = {
			texture,
			glm::ivec2(Fonts[fontName_]->glyph->bitmap.width, Fonts[fontName_]->glyph->bitmap.rows),
			glm::ivec2(Fonts[fontName_]->glyph->bitmap_left, Fonts[fontName_]->glyph->bitmap_top),
			static_cast<GLuint>(Fonts[fontName_]->glyph->advance.x)
		};
		temp->insert(std::pair<GLchar, Character>(*ch, character));
		glBindTexture(GL_TEXTURE_2D, 0);

	}
	//Reset GPU pixel mode
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	return temp;

}

FontHandler::FontHandler()
{
	//Initialize freetype
	if (FT_Init_FreeType(&ft))
		std::cout << "!!ERROR!!" << std::endl << "Could not init FreeType Library" << std::endl;

}


FontHandler::~FontHandler()
{
	//Delete all the font faces
	std::map<std::string, FT_Face>::iterator i;

	for (i = Fonts.begin(); i != Fonts.end(); i++)
		FT_Done_Face(i->second);

	Fonts.clear();
	//Shut down freetype
	FT_Done_FreeType(ft);

}
//Return pointer to singleton
FontHandler * HARIKEN::FontHandler::GetInstance()
{

	if (fontInstance.get() == nullptr)
		fontInstance.reset(new FontHandler);

	return fontInstance.get();

}
