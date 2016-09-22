#include "instructions.h"

#include <glog/logging.h>
#include <iomanip>
#include <stack>
#include <math.h>

#define FOR_EACH_INSTRUCTIONS(V) \
    V(SVTCA,0, 0x00, zero) \
    V(SVTCA,1, 0x01, zero) \
    V(SRP0, 0, 0x10, zero) \
    V(SRP1, 0, 0x11, zero) \
    V(MDAP, 0, 0x2E, zero) \
    V(MDAP, 1, 0x2F, zero) \
    V(IUP,0, 0x30, zero) \
    V(IUP,1, 0x31, zero) \
    V(SHP,0, 0x32, zero) \
    V(SHP,1, 0x33, zero) \
    V(MIAP,0, 0x3E, zero) \
    V(MIAP,1, 0x3F, zero) \
    V(IP,0, 0x39, zero) \
    V(NPUSHB,0, 0x40, consume_n_from_is) \
    V(DELTAP1, 0, 0x5D, zero) \
    V(SDB, 0, 0x5E, zero) \
    V(MIRP,00000, 0xE0, zero) \
    V(MIRP,00001, 0xE1, zero) \
    V(MIRP,00010, 0xE2, zero) \
    V(MIRP,00011, 0xE3, zero) \
    V(MIRP,00100, 0xE4, zero) \
    V(MIRP,00101, 0xE5, zero) \
    V(MIRP,00110, 0xE6, zero) \
    V(MIRP,00111, 0xE7, zero) \
    V(MIRP,01000, 0xE8, zero) \
    V(MIRP,01001, 0xE9, zero) \
    V(MIRP,01010, 0xEA, zero) \
    V(MIRP,01011, 0xEB, zero) \
    V(MIRP,01100, 0xEC, zero) \
    V(MIRP,01101, 0xED, zero) \
    V(MIRP,01110, 0xEE, zero) \
    V(MIRP,01111, 0xEF, zero) \
    V(MIRP,10000, 0xF0, zero) \
    V(MIRP,10001, 0xF1, zero) \
    V(MIRP,10010, 0xF2, zero) \
    V(MIRP,10011, 0xF3, zero) \
    V(MIRP,10100, 0xF4, zero) \
    V(MIRP,10101, 0xF5, zero) \
    V(MIRP,10110, 0xF6, zero) \
    V(MIRP,10111, 0xF7, zero) \
    V(MIRP,11000, 0xF8, zero) \
    V(MIRP,11001, 0xF9, zero) \
    V(MIRP,11010, 0xFA, zero) \
    V(MIRP,11011, 0xFB, zero) \
    V(MIRP,11100, 0xFC, zero) \
    V(MIRP,11101, 0xFD, zero) \
    V(MIRP,11110, 0xFE, zero) \
    V(MIRP,11111, 0xFF, zero) \

namespace {

int zero(const std::vector<uint8_t>& inst, int pc) { return pc; }

int consume_n_from_is(const std::vector<uint8_t>& inst, int pc) {
  int n = inst[pc++];
  std::stringstream ss;
  ss << "[";
  for (size_t i = 0; i < n; ++i) {
    if (i != 0) ss << ", ";
    ss << "0x" << std::setfill('0') << std::setw(2) << std::hex << (uint32_t)inst[pc++];
  }
  ss << "]";
  LOG(ERROR) << n << " data: " << ss.str();
  return pc;
}

struct Vector {
  Vector(double x, double y) : x(x), y(y) {}

  double x;
  double y;
};

struct UnitVector {
  UnitVector(double x, double y) : x_(x), y_(y) {
    normalize();
  }

  void normalize() {
    if (x_ == 0.0 && y_ == 0.0)
      return;

    double r = sqrt(x_ * x_ + y_ * y_);
    x_ /= r;
    y_ /= r;
  }

  double x() const { return x_; }
  double y() const { return y_; }
  void set_x(double x) { x_ = x; normalize(); }
  void set_y(double y) { y_ = y; normalize(); }

 private:
  double x_;
  double y_;
};

struct Environment {
  Environment(const SimpleGlyphData& glyph,
              int grid_size,
              const std::vector<int16_t>& cvt)
      : glyph(glyph), grid_size(grid_size), cvt(cvt),
      contours(glyph.contours),
      pc(0),
      freedom_vector(1, 0),  // x-axis by default
      projection_vector(1, 0), // x-axis by default
      gep0(1), gep1(1), gep2(1)
  {}

  // input
  const SimpleGlyphData& glyph;
  const int grid_size;
  const std::vector<int16_t>& cvt;

  // output
  std::vector<Contour> contours;

  // internal variables
  int pc;
  std::stack<uint32_t> stack;

  UnitVector freedom_vector;
  UnitVector projection_vector;

  int gep0;
  int gep1;
  int gep2;

  // utility functions
  uint8_t nextByte() { return glyph.instructions[pc++]; }
  bool finished() const { return pc >= glyph.instructions.size(); }
};

void SVTCA(int variant,  Environment* env) {
  if (variant == 0) {
    LOG(ERROR) << "SVTCA[0] - Set Projection/Freedom Vector to Y-axis.";
    env->freedom_vector = UnitVector(0, 1);
    env->projection_vector = UnitVector(0, 1);
  } else {
    LOG(ERROR) << "SVTCA[1] - Set Projection/Freedom Vector to X-axis.";
    env->freedom_vector = UnitVector(1, 0);
    env->projection_vector = UnitVector(1, 0);
  }
}

void SRP0(int variant,  Environment* env) {
  LOG(ERROR) << __FUNCTION__;
}

void SRP1(int variant,  Environment* env) {
  LOG(ERROR) << __FUNCTION__;
}

void MDAP(int variant,  Environment* env) {
  LOG(ERROR) << __FUNCTION__;
}

void IUP(int variant,  Environment* env) {
  LOG(ERROR) << __FUNCTION__;
}

void SHP(int variant,  Environment* env) {
  LOG(ERROR) << __FUNCTION__;
}

void MIAP(int variant,  Environment* env) {
  uint32_t n = env->stack.top(); env->stack.pop();
  uint32_t p = env->stack.top(); env->stack.pop();

  LOG(ERROR) << env->cvt[n] << ", " << p;

  int16_t dist = env->cvt[n];

  LOG(ERROR) << "MIAP[" << variant << "] - ";
}

void IP(int variant,  Environment* env) {
  LOG(ERROR) << __FUNCTION__;
}

void NPUSHB(int variant,  Environment* env) {
  int n = env->nextByte();
  LOG(ERROR) << "NPUSHB - Push " << n << " bytes.";
  for (int i = 0; i < n; ++i) {
    env->stack.push(env->nextByte());
  }
}

void DELTAP1(int variant,  Environment* env) {
  LOG(ERROR) << __FUNCTION__;
}

void SDB(int variant,  Environment* env) {
  LOG(ERROR) << __FUNCTION__;
}

void MIRP(int variant,  Environment* env) {
  LOG(ERROR) << __FUNCTION__;
}

}

// static 
std::vector<Contour> HintStackMachine::execute(
    const SimpleGlyphData& glyph,
    int grid_size,
    const std::vector<int16_t>& cvt) {

  Environment env(glyph, grid_size, cvt);
  while (!env.finished()) {
    uint8_t inst = env.nextByte();
    switch (inst) {
#define V(name, variant, opcode, is_move) \
      case opcode: name(variant, &env); break;
      FOR_EACH_INSTRUCTIONS(V)
#undef V
      default:
        LOG(FATAL) << "Uknown opcode pc = " << env.pc <<
            ", 0x" << std::setfill('0') << std::setw(2)
            << std::hex << (uint32_t)inst;
    }
  }
  return env.contours;
}

// static
void HintStackMachine::dumpInstructions(const std::vector<uint8_t>& inst) {
  int pc = 0;
  while (pc < inst.size()) {
    switch (inst[pc++]) {
#define V(name, variant, opcode, is_move) \
      case opcode: LOG(ERROR) << #name << "[" << #variant << "]"; \
                   pc = is_move(inst, pc); break;
      FOR_EACH_INSTRUCTIONS(V)
#undef V
      default:
        LOG(FATAL) << "Uknown opcode pc = " << pc <<
            ", 0x" << std::setfill('0') << std::setw(2)
            << std::hex << (uint32_t)inst[pc -1];
    }
  }
}
