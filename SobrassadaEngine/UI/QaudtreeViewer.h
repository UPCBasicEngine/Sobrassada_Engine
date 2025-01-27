#pragma once

class Framebuffer;

class QaudtreeViewer
{
  public:
    QaudtreeViewer();
    ~QaudtreeViewer();

    void Render(bool& renderBoolean);

  private:
    Framebuffer *framebuffer;
    unsigned int vbo = 0;
    unsigned int program = 0;
};
