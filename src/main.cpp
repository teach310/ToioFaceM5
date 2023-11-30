#include <M5Unified.h>
#include <Avatar.h>

using namespace m5avatar;

Avatar avatar;

const Expression expressions[] = {
  Expression::Angry,
  Expression::Sleepy,
  Expression::Happy,
  Expression::Sad,
  Expression::Doubt,
  Expression::Neutral
};
const int expressionsSize = sizeof(expressions) / sizeof(Expression);
int idx = 0;

void setupAvatar()
{
  avatar.setScale(0.4);
  avatar.setPosition(-56, -96);

  // 目と口のサイズを1.5倍にする
  Face* face = avatar.getFace();
  face->setLeftEye(new Eye(12, false));
  face->setRightEye(new Eye(12, true));
  face->setMouth(new Mouth(50, 90, 6, 60));
  avatar.init();
}

void setup()
{
  auto cfg = M5.config();
  M5.begin(cfg);
  setupAvatar();
}

void loop()
{
  M5.update();
  if (M5.BtnA.wasPressed())
  {
    avatar.setExpression(expressions[idx]);
    idx = (idx + 1) % expressionsSize;
    // 口の開き具合をランダムに変える
    avatar.setMouthOpenRatio(random(1, 10)*0.1);
  }
}