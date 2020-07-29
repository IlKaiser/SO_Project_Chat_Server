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
import so.chat.LoginActivity.LoginActivity
import java.io.BufferedReader
import java.io.InputStreamReader
import java.io.PrintWriter
import java.net.SocketException
import kotlin.system.exitProcess

class MainActivity : AppCompatActivity() {


    override fun onCreate(savedInstanceState: Bundle?) {

        LoginActivity.printWriter=PrintWriter(LoginActivity.socket!!.getOutputStream())
        LoginActivity.reader= BufferedReader(InputStreamReader(LoginActivity.socket!!.getInputStream()))
        val tag ="Main activty"
        Log.d(tag, "onCreate: ")
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        val textView=findViewById<TextView>(R.id.textView)
        val sendButton=findViewById<FloatingActionButton>(R.id.floatingActionButton)
        val textToSend=findViewById<EditText>(R.id.sendText)

        textView.text="Connected"
        textView.movementMethod=ScrollingMovementMethod.getInstance()

        class UpdateThread: Thread(){
            override fun run() {
                //Thread.sleep(2000)
                Looper.prepare()
                val reAlone=".*Alone".toRegex()
                val reList=".*List.*".toRegex()
                val reOk=".*OK.*".toRegex()
                val reAffaf=".*AFFAF.*".toRegex()
                val reMsg=".*H_MSG.*".toRegex()
                val reQuit=".*QUIT.*".toRegex()
                //val reSemi=".*0x.,.*".toRegex()
                Log.d(tag, "run: update td start")
                while(true){
                    try {
                        if(!LoginActivity.socket!!.isClosed) {
                            val line = LoginActivity.reader!!.readLine()
                            val text: String = textView.text.toString()
                            if (line != null && line != "null" && line!=""
                                    && !reOk.containsMatchIn(line)) {
                                println("Got in {$line}")
                                when {
                                    reQuit.containsMatchIn(line) -> {
                                        textView.text = textView.text.toString().plus("User you were chatting with quit the chat," +
                                                "type _LIST_ to choose someone else\n")
                                    }
                                    reAlone.containsMatchIn(line) ->{
                                        textView.text =("You are the only User online!" +
                                                "\nWait until someone connects...\n")
                                    }
                                    reList.containsMatchIn(line) -> {
                                        Log.d(tag, "run: Lista")
                                        textView.text = line.plus("\n")
                                    }
                                    reMsg.containsMatchIn(line) -> {
                                        LoginActivity.printWriter!!.print("H_MSG\n".plus('\u0000'))
                                        LoginActivity.printWriter!!.flush()
                                        textView.text = "Vecchi messaggi:\n"
                                    }
                                    reAffaf.containsMatchIn(line) -> {
                                        Log.d(tag, "run: AFFAF, closing...")
                                        LoginActivity.printWriter!!.print(("_QUIT_\n").plus('\u0000'))
                                        Toast.makeText(applicationContext, "Errore del server", Toast.LENGTH_LONG).show()
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
                    Log.d(tag, "sendText: Got $line")
                    line.plus("\n")
                     if(!reNumber.containsMatchIn(line)) {
                        val view = textView.text.toString()
                        val msg = view.plus("\n").plus("Tu:\n").plus(line)
                        textView.text = msg
                    }else{
                        textView.text=""
                    }
                    LoginActivity.printWriter!!.print(line.plus('\u0000'))
                    LoginActivity.printWriter!!.flush()
                    if(reQuit.containsMatchIn(line)){
                        Log.d(tag, "run:QUIT")
                        Toast.makeText(applicationContext,"Ciao,alla prossima!",Toast.LENGTH_LONG).show()
                        finish()
                        exitProcess(0)
                    }
                }
            }
        }
        sendButton?.setOnClickListener{ val td=SendThread(); td.start()}
    }

    override fun onDestroy(){
        class CloseThread: Thread() {
            override fun run() {
                super.run()
                try {
                    LoginActivity.printWriter!!.print("QUIT\n".plus('\u0000'))
                    LoginActivity.printWriter!!.flush()
                    LoginActivity.printWriter!!.close()
                    LoginActivity.reader!!.close()
                    LoginActivity.socket!!.close()
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