define i32 @main() {
entry:
  %aVar = alloca i32
  %secret = call i32 () @SOURCE()
  store i32 %secret, ptr %aVar
  %cond = icmp eq i32 %secret, 0
  br i1 %cond, label %ifTrue, label %ifFalse

ifTrue:
  store i32 42, ptr %aVar
  br label %end

ifFalse:
  br label %end

end:
  %a1 = load i32, ptr %aVar
  call void @SINK(i32 %a1)
  ret i32 0
}

declare i32 @SOURCE()
declare void @SINK(i32)
