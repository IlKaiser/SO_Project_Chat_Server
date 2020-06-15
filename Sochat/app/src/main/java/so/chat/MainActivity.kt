package so.chat

import android.os.Bundle
import android.os.Looper
import android.text.method.ScrollingMovementMethod
import android.util.Log
import android.widget.EditText
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.google.android.material.floatingactionbutton.FloatingActionButton
import java.io.BufferedReader
import java.io.InputStreamReader
import java.io.PrintWriter
import java.net.Socket
import java.net.SocketException
import kotlin.system.exitProcess

class MainActivity : AppCompatActivity() {
    var socket:Socket? =null
    var reader:BufferedReader? = null
    var printWriter: PrintWriter? =null

    override fun onCreate(savedInstanceState: Bundle?) {
        val TAG ="Main activty"
        Log.d(TAG, "onCreate: ")
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        val textView=findViewById<TextView>(R.id.textView)
        val sendButton=findViewById<FloatingActionButton>(R.id.floatingActionButton)
        val textToSend=findViewById<EditText>(R.id.sendText)

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
                        if(!socket!!.isClosed) {
                            val line = reader!!.readLine()
                            val text: String = textView.text.toString()
                            if (line != null && line != "null" && !reAlone.containsMatchIn(line)
                                    && !reOk.containsMatchIn(line)) {
                                println("Got in {$line}")
                                when {
                                    reList.containsMatchIn(line) -> {
                                        Log.d(TAG, "run: Lista")
                                        textView.text = line.plus("\n")
                                    }
                                    reAffaf.containsMatchIn(line) -> {
                                        Log.d(TAG, "run: AFFAF, closing...")
                                        printWriter!!.println("QUIT\n")
                                        Toast.makeText(applicationContext, "Errore del server riprova pi� tardi", Toast.LENGTH_LONG).show()
                                        finish()
                                        exitProcess(-1)

                                    }
                                    else -> {
                                        textView.text = text.plus("\n").plus(line)
                                    }
                                }
                            }
                        }
                    }catch (e:NullPointerException){}
                     catch (e1:SocketException){

                     }
                }
            }
        }

        val update=UpdateThread()
        update.start()

        class SendThread:Thread(){
            override fun run() {
                Looper.prepare()
                val line=textToSend.text.toString()
                textToSend.setText("")
                val reNumber="[0-9]".toRegex()
                val reQuit=".*QUIT.*".toRegex()
                if(line != "") {
                    println(line)
                    Log.d(TAG, "sendText: Got $line")
                    line.plus("\n")
                     if(!reNumber.containsMatchIn(line)) {
                        val view = textView.text.toString()
                        val msg = view.plus("\n").plus("Tu:\n").plus(line)
                        textView.text = msg
                    }else{
                        textView.text=""
                    }
                    printWriter!!.println(line)
                    printWriter!!.flush()
                    if(reQuit.containsMatchIn(line)){
                        Log.d(TAG, "run:QUIT")
                        Toast.makeText(applicationContext,"Ciao,alla prossima!",Toast.LENGTH_LONG).show()
                        finish()
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
                }catch (e:Exception){super.run()
                    Toast.makeText(applicationContext,"Server su aws offline",Toast.LENGTH_LONG).show()
                    finish()
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

    override fun onDestroy(){
        class CloseThread: Thread() {
            override fun run() {
                super.run()
                try {
                    printWriter!!.println("QUIT\n")
                    printWriter!!.flush()
                    printWriter!!.close()
                    reader!!.close()
                    socket!!.close()
                }catch (e: NullPointerException){}
            }
        }
        super.onDestroy()
        val tdClose=CloseThread()
        tdClose.start()
        tdClose.join()
        finish()
        exitProcess(0)
    }
}