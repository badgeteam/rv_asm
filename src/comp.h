#pragma once

#include<stdlib.h>
#include<stdio.h>
#include<stdbool.h>

#include"token.h"

enum Pass{
  INDEX,
  COMP,
};

typedef struct CompContext{
  struct Token*tokenHead;
  struct Token*token;
  enum Pass pass;
}CompContext;

CompContext*comp(char*filename);
