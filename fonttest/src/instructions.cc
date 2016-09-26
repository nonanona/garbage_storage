#include "instructions.h"

#include "fpgm.h"
#include "cvt.h"
#include "prep.h"
#include "head.h"
#include "truetype.h"

#include <glog/logging.h>
#include <iomanip>
#include <stack>
#include <math.h>
#include <map>

#define FOR_EACH_INSTRUCTIONS(V) \
    V(SVTCA,0, 0x00, zero) \
    V(SVTCA,1, 0x01, zero) \
    V(SPVTCA,0, 0x02, zero) \
    V(SPVTCA,1, 0x03, zero) \
    V(SPVTL,0, 0x06, zero) \
    V(SPVTL,1, 0x07, zero) \
    V(SFVTL,0, 0x08, zero) \
    V(SFVTL,1, 0x09, zero) \
    V(SPVFS,0, 0x0A, zero) \
    V(SFVFS,0, 0x0B, zero) \
    V(GPV,0, 0x0C, zero) \
    V(GFV,0, 0x0D, zero) \
    V(SRP0, 0, 0x10, zero) \
    V(SRP1, 0, 0x11, zero) \
    V(SRP2, 0, 0x12, zero) \
    V(SZPS, 0, 0x16, zero) \
    V(RTG, 0, 0x18, zero) \
    V(ELSE, 0, 0x1B, zero) \
    V(SCVTCI, 0, 0x1D, zero) \
    V(DUP, 0, 0x20, zero) \
    V(POP, 0, 0x21, zero) \
    V(SWAP, 0, 0x23, zero) \
    V(CINDEX, 0, 0x25, zero) \
    V(MINDEX, 0, 0x26, zero) \
    V(CALL, 0, 0x2B, zero) \
    V(FDEF, 0, 0x2C, zero) \
    V(ENDF, 0, 0x2D, zero) \
    V(MDAP, 0, 0x2E, zero) \
    V(MDAP, 1, 0x2F, zero) \
    V(IUP,0, 0x30, zero) \
    V(IUP,1, 0x31, zero) \
    V(SHP,0, 0x32, zero) \
    V(SHP,1, 0x33, zero) \
    V(SHPIX,0, 0x38, zero) \
    V(IP,0, 0x39, zero) \
    V(MSIRP,0, 0x3A, zero) \
    V(MSIRP,1, 0x3B, zero) \
    V(ALIGNRP,1, 0x3C, zero) \
    V(MIAP,0, 0x3E, zero) \
    V(MIAP,1, 0x3F, zero) \
    V(NPUSHB,0, 0x40, consume_n) \
    V(WS,0, 0x42, zero) \
    V(RS,0, 0x43, zero) \
    V(WCVTP,0, 0x44, zero) \
    V(RCVT,0, 0x45, zero) \
    V(GC,0, 0x46, zero) \
    V(GC,1, 0x47, zero) \
    V(SCFS,0, 0x48, zero) \
    V(MD,0, 0x49, zero) \
    V(MD,1, 0x4A, zero) \
    V(MPPEM, 0, 0x4B, zero) \
    V(LT, 0, 0x50, zero) \
    V(LTEQ, 0, 0x51, zero) \
    V(GT, 0, 0x52, zero) \
    V(GTEQ, 0, 0x53, zero) \
    V(EQ, 0, 0x54, zero) \
    V(NEQ, 0, 0x55, zero) \
    V(IF, 0, 0x58, zero) \
    V(EIF, 0, 0x59, zero) \
    V(AND, 0, 0x5A, zero) \
    V(OR, 0, 0x5B, zero) \
    V(NOT, 0, 0x5C, zero) \
    V(DELTAP1, 0, 0x5D, zero) \
    V(SDB, 0, 0x5E, zero) \
    V(SDS, 0, 0x5F, zero) \
    V(ADD, 0, 0x60, zero) \
    V(SUB, 0, 0x61, zero) \
    V(DIV, 0, 0x62, zero) \
    V(MUL, 0, 0x63, zero) \
    V(ABS, 0, 0x64, zero) \
    V(NEG, 0, 0x65, zero) \
    V(FLOOR, 0, 0x66, zero) \
    V(ROUND, 00, 0x68, zero) \
    V(ROUND, 01, 0x69, zero) \
    V(ROUND, 10, 0x6A, zero) \
    V(ROUND, 11, 0x6B, zero) \
    V(WCVTF, 0, 0x70, zero) \
    V(DELTAC, 0, 0x73, zero) \
    V(DELTAC, 0, 0x74, zero) \
    V(DELTAC, 0, 0x75, zero) \
    V(SROUND, 0, 0x76, zero) \
    V(ROFF, 0, 0x7A, zero) \
    V(RUTG, 0, 0x7C, zero) \
    V(RDTG, 0, 0x7D, zero) \
    V(SCANCTRL, 0, 0x85, zero) \
    V(SDPVTL, 0, 0x86, zero) \
    V(SDPVTL, 1, 0x87, zero) \
    V(GETINFO, 0, 0x88, zero) \
    V(ROLL, 0, 0x8A, zero) \
    V(MAX, 0, 0x8B, zero) \
    V(MIN, 0, 0x8C, zero) \
    V(SCANTYPE, 0, 0x8D, zero) \
    V(INSTCTRL, 0, 0x8E, zero) \
    V(PUSHB, 000, 0xB0, consume1) \
    V(PUSHB, 001, 0xB1, consume2) \
    V(PUSHB, 010, 0xB2, consume3) \
    V(PUSHB, 011, 0xB3, consume4) \
    V(PUSHB, 100, 0xB4, consume5) \
    V(PUSHB, 101, 0xB5, consume6) \
    V(PUSHB, 110, 0xB6, consume7) \
    V(PUSHB, 111, 0xB7, consume8) \
    V(PUSHW, 000, 0xB8, consumeW1) \
    V(PUSHW, 001, 0xB9, consumeW2) \
    V(PUSHW, 010, 0xBA, consumeW3) \
    V(PUSHW, 011, 0xBB, consumeW4) \
    V(PUSHW, 100, 0xBC, consumeW5) \
    V(PUSHW, 101, 0xBD, consumeW6) \
    V(PUSHW, 110, 0xBE, consumeW7) \
    V(PUSHW, 111, 0xBF, consumeW8) \
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

int consumeW(const std::vector<uint8_t>& inst, int pc, int n) {
  std::stringstream ss;
  ss << "[";
  for (int i = 0; i < n; ++i) {
    if (i != 0) ss << ", ";
    int high = inst[pc++];
    int low = inst[pc++];
    ss << static_cast<int16_t>(high << 8 | low);
  }
  ss << "]";
  LOG(ERROR) << "data: " << ss.str();
  return pc;
}

int consume(const std::vector<uint8_t>& inst, int pc, int n) {
  std::stringstream ss;
  ss << "[";
  for (int i = 0; i < n; ++i) {
    if (i != 0) ss << ", ";
    ss << (uint32_t)inst[pc++];
  }
  ss << "]";
  LOG(ERROR) << "data: " << ss.str();
  return pc;
}

int consume1(const std::vector<uint8_t>& inst, int pc) { return consume(inst, pc, 1); }
int consume2(const std::vector<uint8_t>& inst, int pc) { return consume(inst, pc, 2); }
int consume3(const std::vector<uint8_t>& inst, int pc) { return consume(inst, pc, 3); }
int consume4(const std::vector<uint8_t>& inst, int pc) { return consume(inst, pc, 4); }
int consume5(const std::vector<uint8_t>& inst, int pc) { return consume(inst, pc, 5); }
int consume6(const std::vector<uint8_t>& inst, int pc) { return consume(inst, pc, 6); }
int consume7(const std::vector<uint8_t>& inst, int pc) { return consume(inst, pc, 7); }
int consume8(const std::vector<uint8_t>& inst, int pc) { return consume(inst, pc, 8); }

int consumeW1(const std::vector<uint8_t>& inst, int pc) { return consumeW(inst, pc, 1); }
int consumeW2(const std::vector<uint8_t>& inst, int pc) { return consumeW(inst, pc, 2); }
int consumeW3(const std::vector<uint8_t>& inst, int pc) { return consumeW(inst, pc, 3); }
int consumeW4(const std::vector<uint8_t>& inst, int pc) { return consumeW(inst, pc, 4); }
int consumeW5(const std::vector<uint8_t>& inst, int pc) { return consumeW(inst, pc, 5); }
int consumeW6(const std::vector<uint8_t>& inst, int pc) { return consumeW(inst, pc, 6); }
int consumeW7(const std::vector<uint8_t>& inst, int pc) { return consumeW(inst, pc, 7); }
int consumeW8(const std::vector<uint8_t>& inst, int pc) { return consumeW(inst, pc, 8); }

int consume_n(const std::vector<uint8_t>& inst, int pc) {
  int n = inst[pc++];
  return consume(inst, pc, n);
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

struct Context;

#define V(name, variant, opcode, is_move) \
  void name(int op, const uint8_t* is, size_t len, Context* ctx, int* pc);
  FOR_EACH_INSTRUCTIONS(V)
#undef V

struct Context {
  Context(const SimpleGlyphData& glyph,
      int grid_size,
      const TrueType& tt)
      : glyph(glyph), grid_size(grid_size),
      contours(glyph.contours),
      freedom_vector(1, 0),  // x-axis by default
      projection_vector(1, 0), // x-axis by default
      gep0(1), gep1(1), gep2(1),
      rp0(0), rp1(0), rp2(0),
      scan_control(false),
      scan_type(0),
      delta_base(9),
      delta_shift(3),
      instruction_control(0),
      round_state(1),
      control_value_cutin(17.0/16.0)
  {
    fpgm = tt.getFpgm();
    cvt = tt.getCvt();
    head = tt.getHead();
  }

  // input
  const SimpleGlyphData& glyph;
  const std::vector<uint8_t>* instructions;
  const int grid_size;

  // output
  std::vector<Contour> contours;

  // internal variables
  std::unique_ptr<FpgmSubTable> fpgm;
  std::unique_ptr<CvtSubTable> cvt;
  std::unique_ptr<HeadSubTable> head;

  std::map<uint8_t, uint32_t> func_map;
  std::map<uint8_t, uint32_t> storage;
  std::stack<uint32_t> stack;

  UnitVector freedom_vector;
  UnitVector projection_vector;

  uint32_t gep0;
  uint32_t gep1;
  uint32_t gep2;

  uint32_t rp0;
  uint32_t rp1;
  uint32_t rp2;

  bool scan_control;
  int scan_type;
  int delta_base;
  int delta_shift;
  uint32_t instruction_control;
  int round_state;
  double control_value_cutin;

  void run(const uint8_t* inst_stream, size_t length) {
    int pc = 0;
    while (pc < length) {
      uint8_t inst = inst_stream[pc++];
      switch (inst) {
#define V(name, variant, opcode, is_move) \
        case opcode: \
        name(opcode, inst_stream, length, this, &pc); break;
        FOR_EACH_INSTRUCTIONS(V)
#undef V
        default:
          LOG(FATAL) << "Uknown opcode pc = " << pc <<
              ", 0x" << std::setfill('0') << std::setw(2)
              << std::hex << (uint32_t)inst;
      }
    }
  }

  GlyphPoint* findPoint(size_t idx) {
    size_t skipped = 0;
    for (size_t i = 0; i < contours.size(); ++i) {
      if (skipped + contours[i].points.size() >= idx) {
        return &contours[i].points[idx - skipped];
      } else {
        skipped += contours[i].points.size();
      }
    }
    LOG(FATAL) << "Invalid position of the glyph point.";
  }
};

void SVTCA(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  if ((opcode & 1) == 0) {
    LOG(ERROR) << "SVTCA[0] - Set Projection/Freedom Vector to Y-axis.";
    ctx->freedom_vector = UnitVector(0, 1);
    ctx->projection_vector = UnitVector(0, 1);
  } else {
    LOG(ERROR) << "SVTCA[1] - Set Projection/Freedom Vector to X-axis.";
    ctx->freedom_vector = UnitVector(1, 0);
    ctx->projection_vector = UnitVector(1, 0);
  }
}

void WS(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t value = ctx->stack.top(); ctx->stack.pop();
  uint32_t location = ctx->stack.top(); ctx->stack.pop();
  ctx->storage[location] = value;
  LOG(ERROR) << "WS: storage[" << location << "] <- " << value;
}

void RS(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t location = ctx->stack.top(); ctx->stack.pop();
  uint32_t value = ctx->storage[location];
  ctx->stack.push(value);
  LOG(ERROR) << "RS: storage[" << location << "] -> " << value;
}

void WCVTP(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t value = ctx->stack.top(); ctx->stack.pop();
  uint32_t location = ctx->stack.top(); ctx->stack.pop();
  LOG(ERROR) << "WCVTP : cvt[" << location << "] <- " << value;
  (*ctx->cvt)[location] = value;
}

void RCVT(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t location = ctx->stack.top(); ctx->stack.pop();
  uint32_t value = (*ctx->cvt)[location];
  LOG(ERROR) << "RCVT: cvt[" << location << "] -> " << value;
  ctx->stack.push(value);
}

void GC(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void SCFS(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void MD(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void MPPEM(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  ctx->stack.push(ctx->head->unit_per_em() / ctx->grid_size);
  LOG(ERROR) << __FUNCTION__ << ": " << ctx->stack.top();
}

void LT(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t left = ctx->stack.top(); ctx->stack.pop();
  uint32_t right = ctx->stack.top(); ctx->stack.pop();
  ctx->stack.push(left > right ? 1 : 0);
  LOG(ERROR) << __FUNCTION__ << " : " << left << ", " << right << " -> " << ctx->stack.top();
}

void LTEQ(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t left = ctx->stack.top(); ctx->stack.pop();
  uint32_t right = ctx->stack.top(); ctx->stack.pop();
  ctx->stack.push(left >= right ? 1 : 0);
  LOG(ERROR) << __FUNCTION__ << " : " << left << ", " << right << " -> " << ctx->stack.top();
}

void GT(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t left = ctx->stack.top(); ctx->stack.pop();
  uint32_t right = ctx->stack.top(); ctx->stack.pop();
  ctx->stack.push(left < right ? 1 : 0);
  LOG(ERROR) << __FUNCTION__ << " : " << left << ", " << right << " -> " << ctx->stack.top();
}

void GTEQ(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t left = ctx->stack.top(); ctx->stack.pop();
  uint32_t right = ctx->stack.top(); ctx->stack.pop();
  ctx->stack.push(left <= right ? 1 : 0);
  LOG(ERROR) << __FUNCTION__ << " : " << left << ", " << right << " -> " << ctx->stack.top();
}

void EQ(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t left = ctx->stack.top(); ctx->stack.pop();
  uint32_t right = ctx->stack.top(); ctx->stack.pop();
  ctx->stack.push(left == right ? 1 : 0);
  LOG(ERROR) << __FUNCTION__ << " : " << left << ", " << right << " -> " << ctx->stack.top();
}

void NEQ(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t left = ctx->stack.top(); ctx->stack.pop();
  uint32_t right = ctx->stack.top(); ctx->stack.pop();
  ctx->stack.push(left != right ? 1 : 0);
  LOG(ERROR) << __FUNCTION__ << " : " << left << ", " << right << " -> " << ctx->stack.top();
}

void IF(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t cond = ctx->stack.top(); ctx->stack.pop();
  if (cond) {
    LOG(ERROR) << "IF : (" << cond << ") jump to " << *pc;
    return;  // Just execute next instruction.
  }

  int if_nested = 1;
  do {
    if (is[*pc] == 0x58 /* IF */) {
      ++if_nested;
    } else if (is[*pc] == 0x1B /* ELSE */ || is[*pc] == 0x59 /* EIF */) {
      --if_nested;
      if (if_nested == 0) {
        break;
      }
    }
  } while ((*pc++) < len);
  if (*pc == len) {
    LOG(FATAL) << "ELSE or EIF not found";
  }
  LOG(ERROR) << "IF : (" << cond << ") jump to " << *pc;
}

void EIF(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << "EIF";
}

void AND(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t left = ctx->stack.top(); ctx->stack.pop();
  uint32_t right = ctx->stack.top(); ctx->stack.pop();
  ctx->stack.push(left && right ? 1 : 0);
  LOG(ERROR) << __FUNCTION__ << " : " << left << ", " << right << " -> " << ctx->stack.top();
}

void OR(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t left = ctx->stack.top(); ctx->stack.pop();
  uint32_t right = ctx->stack.top(); ctx->stack.pop();
  ctx->stack.push(left || right ? 1 : 0);
  LOG(ERROR) << __FUNCTION__ << " : " << left << ", " << right << " -> " << ctx->stack.top();
}

void SZPS(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void RTG(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void ELSE(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  int if_nested = 1;
  do {
    if (is[*pc] == 0x58 /* IF */) {
      ++if_nested;
    } else if (is[*pc] == 0x59 /* EIF */) {
      --if_nested;
      if (if_nested == 0) {
        break;
      }
    }
  } while ((*pc++) < len);
  if (*pc == len) {
    LOG(FATAL) << "EIF not found";
  }
  LOG(ERROR) << "ELSE";
}

void SCVTCI(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t value = ctx->stack.top(); ctx->stack.pop();
  ctx->control_value_cutin = value;
  LOG(ERROR) << __FUNCTION__ << " : " << value;
}

void DUP(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void POP(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << "POP: stack -> " << ctx->stack.top();
  ctx->stack.pop();
}

void SWAP(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void CINDEX(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void MINDEX(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void CALL(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t n = ctx->stack.top(); ctx->stack.pop();
  if (ctx->func_map.find(n) == ctx->func_map.end()) {
    LOG(FATAL) << "Unknown function entry point: " << n;
  }
  LOG(ERROR) << "CALL: " << n << ", " << ctx->func_map[n];
  const std::vector<uint8_t>& fpgm_inst = ctx->fpgm->instructions();
  const uint8_t* entry_point = &fpgm_inst[0] + ctx->func_map[n];
  ctx->run(entry_point, fpgm_inst.size() - ctx->func_map[n]);
  LOG(ERROR) << "CALL " << n << " END";
}

void SPVTCA(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void SPVTL(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void SFVTL(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void SPVFS(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void SFVFS(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void GPV(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void GFV(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void SRP0(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t n = ctx->stack.top(); ctx->stack.pop();
  ctx->rp0 = n;
}

void SRP1(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void SRP2(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void FDEF(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t f_idx = ctx->stack.top(); ctx->stack.pop();
  if (ctx->func_map.find(f_idx) != ctx->func_map.end()) {
    LOG(FATAL) << "Duplicated function entry: " << f_idx;
  }
  ctx->func_map[f_idx] = *pc;
  while (is[(*pc)++] != 0x2D /* EDEF */) {}
}

void ENDF(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  *pc = 1000000000;
}

void MDAP(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void IUP(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void SHP(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void MSIRP(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void ALIGNRP(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void MIAP(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t n = ctx->stack.top(); ctx->stack.pop();
  uint32_t p = ctx->stack.top(); ctx->stack.pop();

  GlyphPoint* point = ctx->findPoint(p);
  LOG(ERROR) << ctx->cvt->cvt()[n] << ", " << point->toString();

  int16_t dist = ctx->cvt->cvt()[n];

  LOG(ERROR) << "MIAP[" << (opcode & 1) << "] - ";
}

void SHPIX(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void IP(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void NPUSHB(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  std::stringstream ss;
  int n = is[(*pc)++];
  for (int i = 0; i < n; ++i) {
    if (i != 0) ss << ", ";
    uint8_t value = is[(*pc)++];

    ctx->stack.push(value);
    ss << (uint16_t) value;
  }
  LOG(ERROR) << "NPUSHB[" << n << "] : " << ss.str();
}

void PUSHB(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  std::stringstream ss;
  for (int i = 0; i <= (opcode - 0xB0); ++i) {
    if (i != 0) ss << ", ";
    uint8_t value = is[(*pc)++];

    ctx->stack.push(value);
    ss << (uint16_t) value;
  }
  LOG(ERROR) << "NPUSHB[" << (opcode - 0xB0) << "] : " << ss.str();
}

void PUSHW(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  std::stringstream ss;
  for (int i = 0; i <= (opcode - 0xB8); ++i) {
    if (i != 0) ss << ", ";
    int16_t high = is[(*pc)++];
    int16_t low = is[(*pc)++];
    int16_t value = high << 8 | low;

    ctx->stack.push(value);
    ss << value;
  }
  LOG(ERROR) << "PUSHW[" << (opcode - 0xB8) << "] : " << ss.str();
}

void NOT(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void DELTAP1(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void SDB(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t value = ctx->stack.top(); ctx->stack.pop();
  ctx->delta_base = value;
  LOG(ERROR) << __FUNCTION__ << ": delta_base <- " << value;
}

void ROLL(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void MAX(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void MIN(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void SCANTYPE(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t n = ctx->stack.top(); ctx->stack.pop();
  LOG(ERROR) << __FUNCTION__ << ": " << n;
  ctx->scan_type = n;
}

void INSTCTRL(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t selector = ctx->stack.top(); ctx->stack.pop();
  int32_t value = ctx->stack.top(); ctx->stack.pop();
  LOG(ERROR) << "INSTCTRL : " << selector << " <- " << value;
  ctx->instruction_control = value;
}

void SDS(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void ADD(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t left = ctx->stack.top(); ctx->stack.pop();
  uint32_t right = ctx->stack.top(); ctx->stack.pop();
  uint32_t value = left + right;
  LOG(ERROR) << __FUNCTION__ << " : " << left/64.0 << ", " << right/64.0 << " -> " << value / 64.0;
  ctx->stack.push(value);
}

void SUB(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t left = ctx->stack.top(); ctx->stack.pop();
  uint32_t right = ctx->stack.top(); ctx->stack.pop();
  uint32_t value = right - left;
  LOG(ERROR) << __FUNCTION__ << " : " << left/64.0 << ", " << right/64.0 << " -> " << value / 64.0;
  ctx->stack.push(value);
}

void DIV(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t left = ctx->stack.top(); ctx->stack.pop();
  uint32_t right = ctx->stack.top(); ctx->stack.pop();
  uint32_t value =  right / left;
  LOG(ERROR) << __FUNCTION__ << " : " << left/64.0 << ", " << right/64.0 << " -> " << value / 64.0;
  ctx->stack.push(value);
}

void MUL(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t left = ctx->stack.top(); ctx->stack.pop();
  uint32_t right = ctx->stack.top(); ctx->stack.pop();
  uint32_t value = left * right / 64;
  LOG(ERROR) << __FUNCTION__ << " : " << left/64.0 << ", " << right/64.0 << " -> " << value / 64.0;
  ctx->stack.push(value);
}

void ABS(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void NEG(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void FLOOR(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void ROUND(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void WCVTF(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void SROUND(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void DELTAC(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t nump = ctx->stack.top(); ctx->stack.pop();
  if (opcode == 0x73) {
    LOG(ERROR) << "DELTAC1 : " << nump;
  } else if (opcode == 0x74) {
    LOG(ERROR) << "DELTAC2 : " << nump;
  } else if (opcode == 0x75) {
    LOG(ERROR) << "DELTAC3 : " << nump;
  }


  uint32_t ppem = ctx->head->unit_per_em() / ctx->grid_size;
  for (size_t i = 0 ; i < nump; ++i) {
    uint32_t arg = ctx->stack.top(); ctx->stack.pop();
    uint32_t c = ctx->stack.top(); ctx->stack.pop();

    uint32_t high = (c >> 4) + ctx->delta_base;
    if (opcode == 0x73 /* DELTAC1 */) {
      // do nothing
    } else if (opcode == 0x74) { /* DELTAC2 */
      high += 16;
    } else if (opcode == 0x75) { /* DELTAC3 */
      high += 32;
    } else {
      LOG(FATAL) << "Not reached.";
    }
    if (high == ppem) {
      int B = (((int32_t)(c)) & 0x0F) - 8;
      if ( B >= 0) {
        B++;
      }
      B *= 1L << (6 - ctx->delta_shift);
      LOG(ERROR) << "DELTAC1 : cvt[" << arg << "] += " << B;
      (*ctx->cvt)[arg] += B;
    }
  }
}

void ROFF(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void RDTG(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void RUTG(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void SCANCTRL(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t n = ctx->stack.top(); ctx->stack.pop();
  uint32_t ppem = ctx->head->unit_per_em() / ctx->grid_size;
  LOG(ERROR) << __FUNCTION__ << " : 0x" << std::hex << n << "(ppem = 0x" << ppem << ")";
  if ((n & 0xFF) == 0xFF) {
    // Alwasys do dropout control
    ctx->scan_control = true;
  } else if ((n & 0xFF) == 0) {
    ctx->scan_control = false;
  } else {
    if ((n & 0x100) && ppem <= (n & 0xFF))
      ctx->scan_control = true;
    if (n & 0x200)
      LOG(FATAL) << "Not yet implemented";
    if (n & 0x400)
      LOG(FATAL) << "Not yet implemented";
    if (n & 0x600)
      LOG(FATAL) << "Not yet implemented";
    if (n & 0x800)
      LOG(FATAL) << "Not yet implemented";
    if (n & 0x1000)
      LOG(FATAL) << "Not yet implemented";
    if (n & 0x2000)
      LOG(FATAL) << "Not yet implemented";
  }
}

void SDPVTL(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

void GETINFO(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  uint32_t request = ctx->stack.top(); ctx->stack.pop();
  uint32_t result = 0;
  if ((request & 1) != 0) {
    result = 35;  // ???
  }
  ctx->stack.push(result);
}

void MIRP(int opcode, const uint8_t* is, size_t len, Context* ctx, int* pc) {
  LOG(ERROR) << __FUNCTION__;
  LOG(FATAL) << "Not implemented.";
}

}

// static 
std::vector<Contour> HintStackMachine::execute(
    const SimpleGlyphData& glyph,
    int grid_size,
    const TrueType& tt) {

  Context ctx(glyph, grid_size, tt);
  std::unique_ptr<FpgmSubTable> fpgm = tt.getFpgm();
  std::unique_ptr<PrepSubTable> prep = tt.getPrep();

  const std::vector<uint8_t>& fpgm_inst = fpgm->instructions();
  const std::vector<uint8_t>& prep_inst = prep->instructions();

  ctx.run(&fpgm_inst[0], fpgm_inst.size());
  ctx.run(&prep_inst[0], prep_inst.size());
  ctx.run(&glyph.instructions[0], glyph.instructions.size());
  return ctx.contours;
}

// static
void HintStackMachine::dumpInstructions(const std::vector<uint8_t>& inst) {
  int pc = 0;
  while (pc < inst.size()) {
    switch (inst[pc++]) {
#define V(name, variant, opcode, is_move) \
      case opcode: LOG(ERROR) << (pc - 1) << ": " << #name << "[" << #variant << "]"; \
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
