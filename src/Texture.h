#pragma once

struct ImageData
{
	std::filesystem::path Filename;
    int Width, Height;
    std::vector<uint32_t> Data;

    ImageData()
    {
        Width = 0; Height = 0;
    }
};

class Texture
{
    friend class ImageLoader;
    static Texture* LastBound;
    bool TextureAssigned;

    void Destroy();
    void CreateTexture();
public:
    Texture(unsigned int texture, int w, int h);
    Texture();
    ~Texture();

    void Bind();
    void LoadFile(std::filesystem::path Filename, bool Regenerate = false);
    void SetTextureData2D(ImageData &Data, bool Reassign = false);

    // Utilitarian
    static void ForceRebind();
    static void Unbind(); // Or, basically unbind.

    // Data
	std::filesystem::path fname;
    int w, h;
    unsigned int texture;
    bool IsValid;
};
