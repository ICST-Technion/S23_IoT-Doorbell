package door.bell.ui.home

import android.R
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.webkit.WebView
import android.webkit.WebViewClient
import android.widget.ProgressBar
import android.widget.TextView
import androidx.fragment.app.Fragment
import androidx.lifecycle.ViewModelProvider
import com.google.android.gms.tasks.OnCompleteListener
import com.google.firebase.messaging.FirebaseMessaging
import door.bell.databinding.FragmentHomeBinding


class HomeFragment : Fragment() {
    private var _binding: FragmentHomeBinding? = null
    private val binding get() = _binding!!

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        val userName = "David"
        val viewModelFactory = HomeViewModelFactory(userName)
        val homeViewModel = ViewModelProvider(this, viewModelFactory).get(HomeViewModel::class.java)


        _binding = FragmentHomeBinding.inflate(inflater, container, false)
        val root: View = binding.root

        val textView: TextView = binding.titleText
        homeViewModel.text.observe(viewLifecycleOwner) {
            textView.text = it
        }

        val webView: WebView = binding.webView
        val progressBar: ProgressBar = binding.progressBar

        webView.webViewClient = object : WebViewClient() {
            override fun onPageFinished(view: WebView?, url: String?) {
                super.onPageFinished(view, url)
                progressBar.visibility = View.GONE
                webView.visibility = View.VISIBLE
            }
        }
        webView.loadUrl("http://192.168.199.132")
        webView.settings.loadWithOverviewMode = true
        webView.settings.useWideViewPort = true
        webView.setInitialScale(1)

        FirebaseMessaging.getInstance().token.addOnCompleteListener(OnCompleteListener { task ->
            if (!task.isSuccessful) {
                Log.w("FCM token", "Fetching FCM registration token failed", task.exception)
                return@OnCompleteListener
            }

            // Get new FCM registration token
            val token = task.result

            // Log and use the registration token as needed
            Log.d("FCM token", "FCM registration token: $token")
        })
        return root
    }


    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}