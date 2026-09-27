package main
import("context";"database/sql";"fmt";"os";"path/filepath";_ "github.com/glebarez/go-sqlite")
func check(file string)error{
 db,err:=sql.Open("sqlite",file+"?_pragma=journal_mode(WAL)");if err!=nil{return err};defer db.Close();db.SetMaxOpenConns(2)
 first,err:=db.Conn(context.Background());if err!=nil{return err};defer first.Close()
 if _,err=first.ExecContext(context.Background(),"CREATE TABLE task_logs (value INTEGER)");err!=nil{return err}
 second,err:=db.Conn(context.Background());if err!=nil{return err};defer second.Close()
 _,err=second.ExecContext(context.Background(),"INSERT INTO task_logs VALUES (1)");return err
}
func main(){
 err:=check(":memory:?cache=shared");fmt.Printf("Default memory, second connection: %v\n",err);if err==nil{panic("negative control unexpectedly succeeded")}
 dir,err:=os.MkdirTemp("","nano-assertoor-db-check-");if err!=nil{panic(err)};defer os.RemoveAll(dir)
 err=check(filepath.Join(dir,"assertoor.sqlite"));fmt.Printf("Per-run file, second connection: %v\n",err);if err!=nil{panic(err)}
}
