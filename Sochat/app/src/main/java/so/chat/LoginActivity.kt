package so.chat

import android.content.Intent
import android.os.Bundle
import android.os.Looper
import android.util.Log
import android.widget.Button
import android.widget.EditText
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import java.io.BufferedReader
import java.io.InputStreamReader
import java.io.PrintWriter
import java.net.Socket
import kotlin.system.exitProcess


class LoginActivity : AppCompatActivity() {

    object LoginActivity{
        var socket: Socket? =null
    }
    var socket=LoginActivity.socket
    var reader: BufferedReader? = null
    var printWriter: PrintWriter? =null


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val intent=Intent(this,MainActivity::class.java)
        val TAG ="Login activty"
        setContentView(R.layout.activity_login)

        val username = findViewById<EditText>(R.id.username)
        val password = findViewById<EditText>(R.id.password)
        val sendButton=findViewById<Button>(R.id.login)


        class SocketThread: Thread(){
            override fun run() {
                Looper.prepare()
                try {
                    LoginActivity.socket = Socket("35.180.35.239", 2015)
                }catch (e:Exception) {
                    super.run()
                    Toast.makeText(applicationContext, "Server su aws offline", Toast.LENGTH_LONG).show()
                    //finish()
                    exitProcess(-1)
                }
                Log.d(TAG, "run: Connected")
                reader=BufferedReader(InputStreamReader(LoginActivity.socket!!.getInputStream()))
                printWriter=PrintWriter(LoginActivity.socket!!.getOutputStream())
                println(reader)
                println(printWriter)
            }
        }

        val socketThread=SocketThread()
        socketThread.start()

        class LoginThread : Thread(){
            override fun run() {
                Looper.prepare()
                super.run()
                val usernameString=username.text.toString()
                val passwordString=password.text.toString()
                val credentials=usernameString.plus(";").plus(passwordString).plus("\n")
                printWriter!!.println(credentials)
                printWriter!!.flush()
                when(val line=reader!!.readLine()){
                    "0xAFFAF" -> {
                        Toast.makeText(applicationContext, "Errore nelle credenziali!", Toast.LENGTH_LONG).show()
                    }
                    "OK" -> {
                        username.setText("")
                        password.setText("")
                        startActivity(intent)
                    }
                    else ->{
                        Toast.makeText(applicationContext, "$line non valido!", Toast.LENGTH_LONG).show()
                    }
                }
                Toast.makeText(applicationContext, "Ciao!", Toast.LENGTH_LONG).show()
            }
        }

        sendButton.setOnClickListener {
            val loginThread=LoginThread()
            loginThread.start()
        }
    }

}

