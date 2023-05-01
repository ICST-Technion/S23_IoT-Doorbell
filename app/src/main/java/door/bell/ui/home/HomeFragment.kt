package door.bell.ui.home

import android.graphics.BitmapFactory
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.webkit.WebView
import android.widget.TextView
import androidx.fragment.app.Fragment
import androidx.lifecycle.ViewModelProvider
import com.google.firebase.storage.FirebaseStorage
import door.bell.databinding.FragmentHomeBinding
import java.io.File

class HomeFragment : Fragment() {
    private lateinit var viewModel: HomeViewModel
    private var _binding: FragmentHomeBinding? = null

    // This property is only valid between onCreateView and
    // onDestroyView.
    private val binding get() = _binding!!

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        val userName = "Rony"
        val viewModelFactory = HomeViewModelFactory(userName)
        val homeViewModel = ViewModelProvider(this, viewModelFactory).get(HomeViewModel::class.java)

        _binding = FragmentHomeBinding.inflate(inflater, container, false)
        val root: View = binding.root

        val textView: TextView = binding.titleText
        homeViewModel.text.observe(viewLifecycleOwner) {
            textView.text = it
        }

        val webView: WebView = binding.webView
        webView.webViewClient = HomeWebViewClient()
        webView.loadUrl("http://google.com/")


        return root
    }


    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}