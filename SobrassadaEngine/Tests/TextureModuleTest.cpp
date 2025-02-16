/*
unsigned int TextureModuleTest::LoadTexture(const wchar_t* texturePath, DirectX::TexMetadata& outTexMetadata)
{
    GLOG("Loading texture: %s", texturePath)
    unsigned int textureId = 0;

    DirectX::ScratchImage scratchImage;
    OpenGLMetadata openGlMeta;

    bool succeded = LoadTextureFile(texturePath, outTexMetadata, scratchImage);

    if (succeded)
    {
        ConvertMetadataOld(outTexMetadata, openGlMeta);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // Sending texture to OpgenGL
        glTexImage2D(
            GL_TEXTURE_2D, 0, openGlMeta.internalFormat, static_cast<GLsizei>(outTexMetadata.width),
            static_cast<GLsizei>(outTexMetadata.height), 0, openGlMeta.format, openGlMeta.type, scratchImage.GetPixels()
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }

    return textureId;
}
*/

