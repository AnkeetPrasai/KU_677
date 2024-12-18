define i32 @main() {
entry:
  %aVar = alloca i32
  %bVar = alloca i32
  %secret = call i32 () @SOURCE()
  %cond = icmp eq i32 1, 1
  br i1 %cond, label %branch1, label %branch2

branch1:
  store i32 %secret, ptr %aVar
  br label %merge

branch2:
  store i32 42, ptr %aVar
  br label %merge

merge:
  %phiVar = phi i32 [42, %branch2], [%secret, %branch1]
  store i32 %phiVar, ptr %bVar
  %b1 = load i32, ptr %bVar
  call void @SINK(i32 %b1)
  ret i32 0
}

declare i32 @SOURCE()
declare void @SINK(i32)
