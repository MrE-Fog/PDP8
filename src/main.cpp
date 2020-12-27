#include "pdp8.h"

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

class Demo_PDP8 : public olc::PixelGameEngine {
  public:
  Demo_PDP8() { sAppName = "PDP8 Demonstration"; }
  
  std::string octal(uint n, uint8_t d) {
    std::string s(d, '0');
    for (int i = d - 1; i >= 0; --i, n >>= 3) {
      s[i] = "01234567"[n & 7];
    }
    return s;
  };
  
  std::string hex(uint n, uint8_t d) {
    std::string s(d, '0');
    for (int i = d - 1; i >= 0; --i, n >>= 4) {
      s[i] = "0123456789ABCDEF"[n & 0xF];
    }
    return s;
  };
  
  std::string binary(uint n, uint8_t d) {
    std::string s(d, '0');
    for (int i = d - 1; i >= 0; --i, n >>= 1) {
      s[i] = "01"[n & 1];
    }
    return s;
  };
  
  void DrawMemory(int x, int y, uint nAddr, int nRows, int nColumns) {
    int nMemX = x, nMemY = y;
    for (int row = 0; row < nRows; ++row) {
      std::string sOffset = " " + format_number(nAddr) + ":";
      for (int col = 0; col < nColumns; ++col) {
        sOffset += " " + format_number(pdp8.memory[nAddr]);
        nAddr += 1;
      }
      DrawString(nMemX, nMemY, sOffset);
      nMemY += 10;
    }
  }
  
  void DrawCPU(int x, int y) {
    DrawString(x,      y, "RUN: ", olc::WHITE);
    DrawString(x + 40, y, std::to_string(pdp8.run), pdp8.run ? olc::GREEN : olc::RED);
    
    DrawString(x, y + 20, "MA: " + format_number(pdp8.ma) + " [" + std::to_string(pdp8.ma) + "]");
    DrawString(x, y + 30, "MB: " + format_number(pdp8.mb) + " [" + std::to_string(pdp8.mb) + "]");
    
    DrawString(x, y + 50, "PC: " + format_number(pdp8.last_pc) + " [" + std::to_string(pdp8.last_pc) + "]");
    DrawString(x, y + 60, "IR: " + format_number(pdp8.ir) + " [" + std::to_string(pdp8.ir) + "]");
    
    DrawString(x,      y + 80, "LINK:", olc::WHITE);
    DrawString(x + 48, y + 80, std::to_string(pdp8.link), pdp8.link ? olc::GREEN : olc::RED);
    DrawString(x,      y + 90, "AC:   " + format_number(pdp8.ac) + " [" + std::to_string(pdp8.ac) + "]");
    
    DrawString(x,       y + 110, "INTERRUPT REQUEST:", olc::WHITE);
    DrawString(x + 152, y + 110, std::to_string(pdp8.interrupt_request), pdp8.interrupt_request ? olc::GREEN : olc::RED);
    DrawString(x,       y + 120, "INTERRUPT ENABLE:", olc::WHITE);
    DrawString(x + 152, y + 120, std::to_string(pdp8.interrupt_enable), pdp8.interrupt_enable ? olc::GREEN : olc::RED);
    
    DrawString(x, y + 140, "SWITCHES: " + binary(pdp8.switches, 12) + " [" + std::to_string(pdp8.switches) + "]");
  }
  
  struct PDP8 pdp8;
  struct PDP8_Program program;
  std::function<std::string(uint)> format_number;
  uint8_t page; // 0 - 31
  
  public:
  bool OnUserCreate() override {
    program = PDP8_BinaryToProgram("../program/compare.bin");
    
    format_number = [&] (uint n) { return octal(n, 4); };
    
    PDP8_Reset(&pdp8);
    PDP8_MemoryReset(&pdp8);
    PDP8_ReloadProgram(&pdp8, &program);
    page = 0;
    
    return true;
  }
  
  bool OnUserUpdate(float fElapsedTime) override {
    Clear(olc::DARK_BLUE);
    
    if (GetKey(olc::Key::SPACE).bPressed) {
      PDP8_Step(&pdp8);
    }
    
    if (GetKey(olc::Key::R).bPressed) {
      PDP8_Reset(&pdp8);
      PDP8_MemoryReset(&pdp8);
      PDP8_ReloadProgram(&pdp8, &program);
    }
    
    if (GetKey(olc::Key::M).bPressed) {
      PDP8_MemoryReset(&pdp8);
    }
    
    if (GetKey(olc::Key::PGDN).bPressed) {
      page++;
      page &= 31;
    }
    
    if (GetKey(olc::Key::PGUP).bPressed) {
      page--;
      page &= 31;
    }
    
    if (GetKey(olc::Key::O).bPressed) {
      format_number = [&] (uint n) { return octal(n, 4); };
    }
    
    if (GetKey(olc::Key::H).bPressed) {
      format_number = [&] (uint n) { return hex(n, 3); };
    }
    
    DrawString(10, 12, "PAGE: " + std::to_string(page));
    DrawMemory(2, 32, page << 7, 16, 8);
    DrawCPU(400, 12);
    
    //DrawString(10, 370, "SPACE = Step Instruction    R = RESET    I = IRQ    N = NMI");
    
    return true;
  }
};


int main() {
  Demo_PDP8 demo;
  if (demo.Construct(680, 480, 2, 2)) {
    demo.Start();
  }
  
  return 0;
}