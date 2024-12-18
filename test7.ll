define i32 @main() {
entry:
  %aVar = alloca i32
  %bVar = alloca i32
  %secret = call i32 () @SOURCE()
  br label %merge

merge:
  %phiVar = phi i32 [%secret, %entry], [0, %merge]
  store i32 %phiVar, ptr %bVar
  %b1 = load i32, ptr %bVar
  call void @SINK(i32 %b1)
  ret i32 0
}

declare i32 @SOURCE()
declare void @SINK(i32)
