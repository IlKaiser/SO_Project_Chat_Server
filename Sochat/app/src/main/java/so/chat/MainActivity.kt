package so.chat

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.text.method.ScrollingMovementMethod
import android.util.Log
import android.view.View
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import android.widget.Toast
import com.google.android.material.floatingactionbutton.FloatingActionButton
import java.io.BufferedReader
import java.io.InputStreamReader
import java.io.PrintWriter
import java.lang.Exception
import java.lang.NullPointerException
import java.net.Socket
import kotlin.system.exitProcess

class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        var TAG="Main activty"
        Log.d(TAG, "onCreate: ")
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        val textView=findViewById<TextView>(R.id.textView)
        val sendButton=findViewById<FloatingActionButton>(R.id.floatingActionButton)
        val textToSend=findViewById<EditText>(R.id.sendText)

        var socket:Socket?
        var reader:BufferedReader? = null
        var printWriter: PrintWriter? =null

        textView.text=""
        textView.movementMethod=ScrollingMovementMethod.getInstance()



        class UpdateThread: Thread(){
            override fun run() {
                //Thread.sleep(2000)
                Looper.prepare()
                val reAlone=".*Alone".toRegex()
                val reList=".*List.*".toRegex()
                val reOk=".*OK.*".toRegex()
                val reAffaf=".*AFFAF.*".toRegex()
                Log.d(TAG, "run: update td start")
                while(true){
                    try {
                        var line = reader!!.readLine()
                        var text: String = textView.text.toString()
                        if (line !=null && line != "null" && !reAlone.containsMatchIn(line)
                                && !reOk.containsMatchIn(line)) {
                            println("Got in {$line}")
                            if(reList.containsMatchIn(line)){
                                Log.d(TAG, "run: Lista")
                                textView.text=line.plus("\n")
                            }
                            else if(reAffaf.containsMatchIn(line)){
                                Log.d(TAG, "run: AFFAF, closing...")
                                printWriter!!.println("QUIT\n");
                                Toast.makeText(applicationContext,"Errore del server riprova pi� tardi",Toast.LENGTH_LONG).show()
                                exitProcess(-1)

                            }
                            else{
                                textView.text = text.plus("\n").plus(line)
                            }
                        }
                    }catch (e:NullPointerException){}
                }
            }
        }

        val update=UpdateThread()
        update.start()

        class SendThread:Thread(){

            override fun run() {
                Looper.prepare()
                var line=textToSend.text.toString()
                textToSend.setText("")
                val reNumber="[0-9]".toRegex()
                val reQuit=".*QUIT.*".toRegex()
                if(line != "") {
                    println(line)
                    Log.d(TAG, "sendText: Got $line")
                    line.plus("\n")
                     if(!reNumber.containsMatchIn(line)) {
                        var view = textView.text.toString()
                        var msg = view.plus("\n").plus("Tu:\n").plus(line)
                        textView.text = msg
                    }else{
                        textView.text=""
                    }
                    printWriter!!.println(line)
                    printWriter!!.flush()
                    if(reQuit.containsMatchIn(line)){
                        Log.d(TAG, "run: QUIT")
                        Toast.makeText(applicationContext,"Ciao,alla prossima",Toast.LENGTH_LONG).show()
                        exitProcess(0)
                    }
                }
            }
        }

        sendButton?.setOnClickListener{ val td=SendThread(); td.start()}

        class SocketThread: Thread(){
            override fun run() {
                Looper.prepare()
                try {
                    socket = Socket("35.180.35.239", 2015)
                }catch (e:Exception){
                    Toast.makeText(applicationContext,"Server su aws offline",Toast.LENGTH_LONG).show()
                    exitProcess(-1)
                }
                textView.text="Connesso"
                Log.d(TAG, "run: Connected")
                reader=BufferedReader(InputStreamReader(socket!!.getInputStream()))
                printWriter=PrintWriter(socket!!.getOutputStream())

                println(reader)
                println(printWriter)
            }
        }
        val socketTd=SocketThread()
        socketTd.start()
    }
}